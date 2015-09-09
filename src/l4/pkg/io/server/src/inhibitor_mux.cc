#include "inhibitor_mux.h"

#include <cstring>

Inhibitor_provider::Inhibitor_provider(Inhibitor_mux *mux)
: _mux(mux)
{
  _mux->_clients.add(this);
  for (unsigned i = 0; i < L4VBUS_INHIBITOR_MAX; ++i)
    {
      _inhibitor_acquired[i] = false;
      _inhibitor_reason[i][0] = 0;
    }
}

Inhibitor_provider::~Inhibitor_provider()
{
  _mux->_clients.remove(this);

  for (unsigned i = 0; i < L4VBUS_INHIBITOR_MAX; ++i)
    if (_inhibitor_acquired[i])
      _mux->inhibitor_release(i);
}

/**
 * \brief Take an inhibitor lock and subscribe the client to the
 *        corresponding signal.
 *
 * \param id     Id of inhibitor to take.
 * \param reason Reason why the client holds the lock.
 */
void
Inhibitor_provider::inhibitor_acquire(L4::Ipc::Istream &in)
{
  auto id = L4::Ipc::read<l4_umword_t>(in);

  if (id >= L4VBUS_INHIBITOR_MAX)
    throw -L4_ERANGE;

  if (_inhibitor_acquired[id])
    return; // already acquired, just ignore

  unsigned long len = sizeof (_inhibitor_reason[id]);
  in >> L4::Ipc::buf_cp_in(_inhibitor_reason[id], len);

  if (len == 0)
    len = 1;

  _inhibitor_reason[id][len - 1] = 0;
  _inhibitor_acquired[id] = true;
  _mux->inhibitor_acquire(id);
}

void
Inhibitor_provider::inhibitor_acquire(l4_umword_t id, char const *reason)
{
  if (id >= L4VBUS_INHIBITOR_MAX)
    throw -L4_ERANGE;

  if (_inhibitor_acquired[id])
    return; // already acquired, just ignore

  unsigned long len = sizeof (_inhibitor_reason[id]);
  strncpy(_inhibitor_reason[id], reason, len);
  _inhibitor_reason[id][len - 1] = 0;
  _inhibitor_acquired[id] = true;
  _mux->inhibitor_acquire(id);
}

/**
 * \brief Release an inhibitor lock and unsubscribe the client from the
 *        corresponding signal.
 *
 * \param id Id of inhibitor lock to release.
 */
void
Inhibitor_provider::inhibitor_release(l4_umword_t id)
{
  if (id >= L4VBUS_INHIBITOR_MAX)
    throw -L4_ERANGE;

  if (!_inhibitor_acquired[id])
    return; // not acquired, just ignore

  _inhibitor_acquired[id] = false;
  _inhibitor_reason[id][0] = 0;
  _mux->inhibitor_release(id);
}

/**
 * \brief Tell the mux that a client has taken a lock.
 *
 * Called by the Inhibitor_provider.
 *
 * \param id Id of lock that was taken.
 */
void
Inhibitor_mux::inhibitor_acquire(l4_umword_t id)
{
  if (id >= L4VBUS_INHIBITOR_MAX)
    throw -L4_ERANGE;

  ++_inhibitors_acquired[id];
}

/**
 * \brief Tell the mux that a client released a lock.
 *
 * Called by the Inhibitor_provider. If the released lock was the last of its
 * kind being held, all_inhibitors_free(type) will be called.
 *
 * \param id ID of the inhibitor_lock that was released.
 */
void
Inhibitor_mux::inhibitor_release(l4_umword_t id)
{
  if (id >= L4VBUS_INHIBITOR_MAX)
    throw -L4_ERANGE;

  if (!--_inhibitors_acquired[id])
    all_inhibitors_free(id);
}

