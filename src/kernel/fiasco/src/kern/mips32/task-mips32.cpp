/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Sanjay Lal <sanjayl@kymasys.com>
 */

IMPLEMENTATION [mips32]:

PRIVATE virtual
bool
Task::invoke_arch(L4_msg_tag &, Utcb *)
{
  return false;
}
