/*
 * Copyright (C) 2014 Imagination Technologies Ltd.
 * Author: Sanjay Lal <sanjayl@kymasys.com>
 * Author: Yann Le Du <ledu@kymasys.com>
 */

INTERFACE [mips32]:

EXTENSION class Io
{
private:
  static inline void io_sync();
};

IMPLEMENTATION [mips32]:

IMPLEMENT static inline ALWAYS_INLINE
void Io::io_sync()
{
  asm volatile ("sync" : : : "memory");
}

IMPLEMENT inline
void Io::iodelay()
{}

IMPLEMENT inline NEEDS[Io::io_sync]
Unsigned8  Io::in8(unsigned long port)
{
  Unsigned8 data = *(volatile Unsigned8 *)port;
  io_sync();
  return data;
}

IMPLEMENT inline NEEDS[Io::io_sync]
Unsigned16 Io::in16( unsigned long port )
{
  Unsigned16 data = *(volatile Unsigned16 *)port;
  io_sync();
  return data;
}

IMPLEMENT inline NEEDS[Io::io_sync]
Unsigned32 Io::in32(unsigned long port)
{
  Unsigned32 data = *(volatile Unsigned32 *)port;
  io_sync();
  return data;
}

IMPLEMENT inline NEEDS[Io::io_sync]
void Io::out8 (Unsigned8  val, unsigned long port)
{
  *(volatile Unsigned8 *)port = val;
  io_sync();
}

IMPLEMENT inline NEEDS[Io::io_sync]
void Io::out16( Unsigned16 val, unsigned long port)
{
  *(volatile Unsigned16 *)port = val;
  io_sync();
}

IMPLEMENT inline NEEDS[Io::io_sync]
void Io::out32(Unsigned32 val, unsigned long port)
{
  *(volatile Unsigned32 *)port = val;
  io_sync();
}
