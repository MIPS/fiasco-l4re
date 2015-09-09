
-- Include L4 functionality
require("L4");

-- Create a channel from the client to the server
local channel = L4.default_loader:new_channel();

-- Start the server, giving the channel with full server rights.
-- The server will have a yellow log output.
L4.default_loader:start(
  {
    caps = { shm = channel:svr() },
    log  = { "server", "yellow" }
  },
  "rom/ex_l4re_ds_srv"
);

-- Start the client, giving it the channel with read only rights. The
-- log output will be green.
L4.default_loader:start(
  {
    caps = { shm = channel },
    log  = { "client", "green"  },
    l4re_dbg = L4.Dbg.Warn
  },
  "rom/ex_l4re_ds_clnt"
);
