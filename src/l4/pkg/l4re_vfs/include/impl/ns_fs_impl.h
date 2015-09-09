/*
 * (c) 2010 Adam Lackorzynski <adam@os.inf.tu-dresden.de>,
 *          Alexander Warg <warg@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 */
#include "ns_fs.h"
#include "vfs_api.h"
#include "ro_file.h"

#include <l4/re/dataspace>
#include <l4/re/util/env_ns>
#include <dirent.h>

namespace L4Re { namespace Core {

Simple_store_sz<Env_dir::Size> Ns_base_dir::store __attribute__((init_priority(1000)));

void *
Ns_base_dir::operator new(size_t s) throw()
{
  if (s > Size)
    return 0;

  return store.alloc();
}

void
Ns_base_dir::operator delete(void *b) throw()
{
  store.free(b);
}


int
Ns_dir::get_ds(const char *path, L4Re::Auto_cap<L4Re::Dataspace>::Cap *ds) throw()
{
  L4Re::Auto_cap<L4Re::Dataspace>::Cap file(cap_alloc()->alloc<L4Re::Dataspace>(), cap_alloc());

  if (!file.is_valid())
    return -ENOMEM;

  int err = _ns->query(path, file.get());

  if (err < 0)
    return -ENOENT;

  *ds = file;
  return err;
}

int
Ns_dir::get_entry(const char *path, int flags, mode_t mode,
                  Ref_ptr<L4Re::Vfs::File> *f) throw()
{
  (void)mode; (void)flags;
  if (!*path)
    {
      *f = this;
      return 0;
    }

  L4Re::Auto_cap<Dataspace>::Cap file;
  int err = get_ds(path, &file);

  if (err < 0)
    return -ENOENT;

  // FIXME: should check if it is a dataspace, somehow
  L4Re::Vfs::File *fi = 0;

  L4::Cap<L4Re::Namespace> nsc
    = L4::cap_dynamic_cast<L4Re::Namespace>(file.get());

  if (!nsc.is_valid())
    fi = new Ro_file(file.get());
  else // use mat protocol here!!
    fi = new Ns_dir(nsc);

  if (!fi)
    return -ENOMEM;

  file.release();
  *f = fi;
  return 0;
}

int
Ns_dir::faccessat(const char *path, int mode, int flags) throw()
{
  (void)flags;
  L4Re::Auto_cap<void>::Cap tmpcap(cap_alloc()->alloc<void>(), cap_alloc());

  if (!tmpcap.is_valid())
    return -ENOMEM;

  if (_ns->query(path, tmpcap.get()))
    return -ENOENT;

  if (mode & W_OK)
    return -EACCES;

  return 0;
}

int
Ns_dir::fstat64(struct stat64 *b) const throw()
{
  b->st_dev = 1;
  b->st_ino = 1;
  b->st_mode = S_IRWXU | S_IFDIR;
  b->st_nlink = 0;
  b->st_uid = 0;
  b->st_gid = 0;
  b->st_rdev = 0;
  b->st_size = 0;
  b->st_blksize = 0;
  b->st_blocks = 0;
  b->st_atime = 0;
  b->st_mtime = 0;
  b->st_ctime = 0;
  return 0;
}

ssize_t
Ns_dir::getdents(char *buf, size_t sz) throw()
{
  struct dirent64 *d = (struct dirent64 *)buf;
  ssize_t ret = 0;
  l4_addr_t infoaddr;
  size_t infosz;

  L4Re::Auto_cap<Dataspace>::Cap dirinfofile;
  int err = get_ds(".dirinfo", &dirinfofile);
  if (err)
    return 0;

  infosz = dirinfofile->size();
  if (infosz <= 0)
    return 0;

  infoaddr = L4_PAGESIZE;
  err = L4Re::Env::env()->rm()->attach(&infoaddr, infosz,
                                       Rm::Search_addr | Rm::Read_only,
                                       dirinfofile.get(), 0);
  char *p   = (char *)infoaddr + _current_dir_pos;
  char *end = (char *)infoaddr + infosz;

  while (d && p < end)
    {
      // parse lines of dirinfofile
      long len;
      for (len = 0; p < end && *p >= '0' && *p <= '9'; ++p)
        {
          len *= 10;
          len += *p - '0';
        }
      if (len)
        {
          // skip colon
          p++;
          if (p + len >= end)
            return 0; // error in dirinfofile

          unsigned l = len + 1;
          if (l > sizeof(d->d_name))
            l = sizeof(d->d_name);

          unsigned n = offsetof (struct dirent64, d_name) + l;
          n = (n + sizeof(long) - 1) & ~(sizeof(long) - 1);

          if (n > sz)
            break;

          d->d_ino = 1;
          d->d_off = 0;
          memcpy(d->d_name, p, len);
          d->d_name[l - 1] = 0;
          d->d_reclen = n;
          d->d_type   = DT_REG;
          ret += n;
          sz  -= n;
          d    = (struct dirent64 *)((unsigned long)d + n);
        }

      // next infodirfile line
      while (p < end && *p && *p != '\n' && *p != '\r')
        p++;
      while (p < end && *p && (*p == '\n' || *p == '\r'))
        p++;
    }

  _current_dir_pos += p - (char *)infoaddr;

  if (!ret) // hack since we should only reset this at open times
    _current_dir_pos = 0;

  L4Re::Env::env()->rm()->detach(infoaddr, 0);

  return ret;
}

int
Env_dir::get_ds(const char *path, L4Re::Auto_cap<L4Re::Dataspace>::Cap *ds) throw()
{
  Vfs::Path p(path);
  Vfs::Path first = p.strip_first();

  if (first.empty())
    return -ENOENT;

  L4::Cap<L4Re::Namespace>
    c = _env->get_cap<L4Re::Namespace>(first.path(), first.length());

  if (!c.is_valid())
    return -ENOENT;

  if (p.empty())
    {
      *ds = L4Re::Auto_cap<L4Re::Dataspace>::Cap(L4::cap_reinterpret_cast<L4Re::Dataspace>(c));
      return 0;
    }

  L4Re::Auto_cap<L4Re::Dataspace>::Cap file(cap_alloc()->alloc<L4Re::Dataspace>(), cap_alloc());

  if (!file.is_valid())
    return -ENOMEM;

  int err = c->query(p.path(), p.length(), file.get());

  if (err < 0)
    return -ENOENT;

  *ds = file;
  return err;
}

int
Env_dir::get_entry(const char *path, int flags, mode_t mode,
                   Ref_ptr<L4Re::Vfs::File> *f) throw()
{
  (void)mode; (void)flags;
  if (!*path)
    {
      *f = this;
      return 0;
    }

  L4Re::Auto_cap<Dataspace>::Cap file;
  int err = get_ds(path, &file);

  if (err < 0)
    return -ENOENT;

  // FIXME: should check if it is a dataspace, somehow
  L4Re::Vfs::File *fi = 0;

  L4::Cap<L4Re::Namespace> nsc
    = L4::cap_dynamic_cast<L4Re::Namespace>(file.get());

  if (!nsc.is_valid())
    fi = new Ro_file(file.get());
  else // use mat protocol here!!
    fi = new Ns_dir(nsc);

  if (!fi)
    return -ENOMEM;

  file.release();
  *f = fi;
  return 0;
}

int
Env_dir::faccessat(const char *path, int mode, int flags) throw()
{
  (void)flags;
  Vfs::Path p(path);
  Vfs::Path first = p.strip_first();

  if (first.empty())
    return -ENOENT;

  L4::Cap<L4Re::Namespace>
    c = _env->get_cap<L4Re::Namespace>(first.path(), first.length());

  if (!c.is_valid())
    return -ENOENT;

  if (p.empty())
    {
      if (mode & W_OK)
	return -EACCES;

      return 0;
    }

  L4Re::Auto_cap<void>::Cap tmpcap(cap_alloc()->alloc<void>(), cap_alloc());

  if (!tmpcap.is_valid())
    return -ENOMEM;

  if (c->query(p.path(), p.length(), tmpcap.get()))
    return -ENOENT;

  if (mode & W_OK)
    return -EACCES;

  return 0;
}

bool
Env_dir::check_type(Env::Cap_entry const *e, long protocol)
{
  L4::Cap<L4::Meta> m(e->cap);
  return m->supports(protocol).label();
}

int
Env_dir::fstat64(struct stat64 *b) const throw()
{
  b->st_dev = 1;
  b->st_ino = 1;
  b->st_mode = S_IRWXU | S_IFDIR;
  b->st_nlink = 0;
  b->st_uid = 0;
  b->st_gid = 0;
  b->st_rdev = 0;
  b->st_size = 0;
  b->st_blksize = 0;
  b->st_blocks = 0;
  b->st_atime = 0;
  b->st_mtime = 0;
  b->st_ctime = 0;
  return 0;
}

ssize_t
Env_dir::getdents(char *buf, size_t sz) throw()
{
  struct dirent64 *d = (struct dirent64 *)buf;
  ssize_t ret = 0;

  while (d
         && _current_cap_entry
         && _current_cap_entry->flags != ~0UL)
    {
      unsigned l = strlen(_current_cap_entry->name) + 1;
      if (l > sizeof(d->d_name))
        l = sizeof(d->d_name);

      unsigned n = offsetof (struct dirent64, d_name) + l;
      n = (n + sizeof(long) - 1) & ~(sizeof(long) - 1);

      if (n <= sz)
        {
          d->d_ino = 1;
          d->d_off = 0;
          memcpy(d->d_name, _current_cap_entry->name, l);
          d->d_name[l - 1] = 0;
          d->d_reclen = n;
          if (check_type(_current_cap_entry, L4Re::Protocol::Namespace))
            d->d_type = DT_DIR;
          else if (check_type(_current_cap_entry, L4Re::Protocol::Dataspace))
            d->d_type = DT_REG;
          else
            d->d_type = DT_UNKNOWN;
          ret += n;
          sz  -= n;
          d    = (struct dirent64 *)((unsigned long)d + n);
          _current_cap_entry++;
        }
      else
        return ret;
    }

  // bit of a hack because we should only (re)set this when opening the dir
  if (!ret)
    _current_cap_entry = _env->initial_caps();

  return ret;
}

}}
