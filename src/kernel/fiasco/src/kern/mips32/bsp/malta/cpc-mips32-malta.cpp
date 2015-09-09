/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Yann Le Du <ledu@kymasys.com>
 */

IMPLEMENTATION [mips32 && malta && cps]:

EXTENSION class Cpc
{
public:
  enum {
    CPC_BASE_ADDR = 0x1bde0000,
  };
};

PRIVATE
phys_t Cpc::mips_cpc_default_phys_base(void)
{
	return CPC_BASE_ADDR;
}
