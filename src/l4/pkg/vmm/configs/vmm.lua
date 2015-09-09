module("vmm", package.seeall);

require("L4");

local l = L4.Loader.new({factory = L4.Env.factory, mem = L4.Env.user_factory});
loader = l;

function new_sched(prio, cpus)
  return l.sched_fab:create(L4.Proto.Scheduler, prio + 10, prio, cpus);
end

function start_io(busses, opts)
  local caps = {
    sigma0 = L4.cast(L4.Proto.Factory, L4.Env.sigma0):create(L4.Proto.Sigma0);
    icu    = L4.Env.icu;
  };

  local files = "";

  for k, v in pairs(busses) do
    local c = l:new_channel();
    busses[k] = c
    caps[k] = c:svr();
    files = files .. " rom/" .. k .. ".vbus";
  end

  return l:start({
    log = { "io", "red" },
    caps = caps
  }, "rom/io " .. opts .. files)
end

local function set_sched(opts, prio, cpus)
  if prio ~= nil then
    local sched = new_sched(prio, cpus);
    opts["scheduler"] = sched;
  end
end

function start_virtio_switch(ports, prio, cpus)
  local caps = {};
  local port_names = "";
  for k, v in pairs(ports) do
    local c = l:new_channel();
    ports[k] = c;
    caps[k] = c:svr();
    port_names = port_names .. " " .. k;
  end

  local opts = {
    log = { "switch", "Blue" },
    caps = caps;
  };

  set_sched(opts, prio, cpus);
  return l:start(opts, "rom/virtio-switch" .. port_names);
end

function start_vm(options)
  local nr      = options.id;
  local size_mb = options.mem or 16;
  local vbus    = options.vbus;
  local vnet    = options.net;
  local prio    = options.prio or 0x10;
  local cpus    = options.cpus;

  local cmdline = {};
  if options.fdt then
    cmdline[#cmdline+1] = "-d" .. options.fdt;
  end

  if options.bootargs then
    cmdline[#cmdline+1] = "-c" .. options.bootargs;
  end

  if options.rd then
    cmdline[#cmdline+1] = "-r" .. options.rd;
  end

  local keyb_shortcut = nil;
  if nr ~= nil then
    keyb_shortcut = "key=" .. nr;
  end

  local caps = {
    net  = vnet;
    vm_bus = vbus;
    ram  = L4.Env.user_factory:create(L4.Proto.Dataspace, size_mb * 1024 * 1024, 0x3, 28):m("rws");
  };

  local opts = {
    log  = l.log_fab:create(L4.Proto.Log, "vm" .. nr, "w", keyb_shortcut);
    caps = caps;
  };

  set_sched(opts, prio, cpus);
  return l:startv(opts, "rom/vmm", unpack(cmdline));
end

