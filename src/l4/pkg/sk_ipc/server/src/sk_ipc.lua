require("L4");

local channel1 = L4.default_loader:new_channel();
local channel2 = L4.default_loader:new_channel();

L4.default_loader:start(
  {
    caps = { trustlet1 = channel1:svr() },
    log = { "Tlet1", "yellow" }
  },
  "rom/trustlet_srv1"
);


L4.default_loader:start(
  {
    caps = { trustlet2 = channel2:svr() },
    log = { "Tlet2", "red" }
  },
  "rom/trustlet_srv2"
);

L4.default_loader:start(
  {
    caps = { trustlet1 = channel1 , trustlet2 = channel2 },
    log = { "tee_clnt", "green" }
  },
  "rom/tee_clnt"
);
