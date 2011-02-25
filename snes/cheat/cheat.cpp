#include <snes/snes.hpp>

#define CHEAT_CPP
namespace SNES {

Cheat cheat;

bool Cheat::enabled() const {
  return system_enabled;
}

void Cheat::enable(bool state) {
  system_enabled = state;
  cheat_enabled = system_enabled && code_enabled;
}

void Cheat::synchronize() {
  memcpy(bus.lookup, lookup, 16 * 1024 * 1024);
  code_enabled = false;

  for(unsigned i = 0; i < size(); i++) {
    const CheatCode &code = operator[](i);
    if(code.enabled == false) continue;

    for(unsigned n = 0; n < code.addr.size(); n++) {
      code_enabled = true;

      unsigned addr = mirror(code.addr[n]);
      bus.lookup[addr] = 0xff;
      if((addr & 0xffe000) == 0x7e0000) {
        //mirror $7e:0000-1fff to $00-3f|80-bf:0000-1fff
        unsigned mirroraddr;
        for(unsigned x = 0; x <= 0x3f; x++) {
          mirroraddr = ((0x00 + x) << 16) + (addr & 0x1fff);
          bus.lookup[mirroraddr] = 0xff;

          mirroraddr = ((0x80 + x) << 16) + (addr & 0x1fff);
          bus.lookup[mirroraddr] = 0xff;
        }
      }
    }
  }

  cheat_enabled = system_enabled && code_enabled;
}

uint8 Cheat::read(unsigned addr) const {
  addr = mirror(addr);

  for(unsigned i = 0; i < size(); i++) {
    const CheatCode &code = operator[](i);
    if(code.enabled == false) continue;

    for(unsigned n = 0; n < code.addr.size(); n++) {
      if(addr == mirror(code.addr[n])) {
        return code.data[n];
      }
    }
  }

  return 0x00;
}

uint8 Cheat::default_reader(unsigned addr) {
  bus.reader[cheat.lookup[addr]](bus.target[addr]);
  return cheat.read(addr);
}

void Cheat::default_writer(unsigned addr, uint8 data) {
  return bus.writer[cheat.lookup[addr]](bus.target[addr], data);
}

void Cheat::init() {
  bus.reader[0xff] = function<uint8(unsigned)>(&Cheat::default_reader);
  bus.writer[0xff] = function<void(unsigned, uint8)>(&Cheat::default_writer);

  memcpy(lookup, bus.lookup, 16 * 1024 * 1024);
}

Cheat::Cheat() {
  lookup = new uint8[16 * 1024 * 1024];
  system_enabled = true;
}

Cheat::~Cheat() {
  delete[] lookup;
}

//===============
//encode / decode
//===============

bool Cheat::decode(const char *s, unsigned &addr, uint8 &data, Type &type) {
  string t = s;
  t.lower();

  #define ischr(n) ((n >= '0' && n <= '9') || (n >= 'a' && n <= 'f'))

  if(strlen(t) == 8 || (strlen(t) == 9 && t[6] == ':')) {
    //strip ':'
    if(strlen(t) == 9 && t[6] == ':') t = string() << substr(t, 0, 6) << substr(t, 7);
    //validate input
    for(unsigned i = 0; i < 8; i++) if(!ischr(t[i])) return false;

    type.i = Type::ProActionReplay;
    unsigned r = hex((const char*)t);
    addr = r >> 8;
    data = r & 0xff;
    return true;
  } else if(strlen(t) == 9 && t[4] == '-') {
    //strip '-'
    t = string() << substr(t, 0, 4) << substr(t, 5);
    //validate input
    for(unsigned i = 0; i < 8; i++) if(!ischr(t[i])) return false;

    type.i = Type::GameGenie;
    t.transform("df4709156bc8a23e", "0123456789abcdef");
    unsigned r = hex((const char*)t);
    //8421 8421 8421 8421 8421 8421
    //abcd efgh ijkl mnop qrst uvwx
    //ijkl qrst opab cduv wxef ghmn
    addr = (!!(r & 0x002000) << 23) | (!!(r & 0x001000) << 22)
         | (!!(r & 0x000800) << 21) | (!!(r & 0x000400) << 20)
         | (!!(r & 0x000020) << 19) | (!!(r & 0x000010) << 18)
         | (!!(r & 0x000008) << 17) | (!!(r & 0x000004) << 16)
         | (!!(r & 0x800000) << 15) | (!!(r & 0x400000) << 14)
         | (!!(r & 0x200000) << 13) | (!!(r & 0x100000) << 12)
         | (!!(r & 0x000002) << 11) | (!!(r & 0x000001) << 10)
         | (!!(r & 0x008000) <<  9) | (!!(r & 0x004000) <<  8)
         | (!!(r & 0x080000) <<  7) | (!!(r & 0x040000) <<  6)
         | (!!(r & 0x020000) <<  5) | (!!(r & 0x010000) <<  4)
         | (!!(r & 0x000200) <<  3) | (!!(r & 0x000100) <<  2)
         | (!!(r & 0x000080) <<  1) | (!!(r & 0x000040) <<  0);
    data = r >> 24;
    return true;
  } else {
    return false;
  }

  #undef ischr
}

bool Cheat::encode(string &s, unsigned addr, uint8 data, Type type) {
  char t[16];

  if(type.i == Type::ProActionReplay) {
    s = string(hex<6>(addr), hex<2>(data));
    return true;
  } else if(type.i == Type::GameGenie) {
    unsigned r = addr;
    addr = (!!(r & 0x008000) << 23) | (!!(r & 0x004000) << 22)
         | (!!(r & 0x002000) << 21) | (!!(r & 0x001000) << 20)
         | (!!(r & 0x000080) << 19) | (!!(r & 0x000040) << 18)
         | (!!(r & 0x000020) << 17) | (!!(r & 0x000010) << 16)
         | (!!(r & 0x000200) << 15) | (!!(r & 0x000100) << 14)
         | (!!(r & 0x800000) << 13) | (!!(r & 0x400000) << 12)
         | (!!(r & 0x200000) << 11) | (!!(r & 0x100000) << 10)
         | (!!(r & 0x000008) <<  9) | (!!(r & 0x000004) <<  8)
         | (!!(r & 0x000002) <<  7) | (!!(r & 0x000001) <<  6)
         | (!!(r & 0x080000) <<  5) | (!!(r & 0x040000) <<  4)
         | (!!(r & 0x020000) <<  3) | (!!(r & 0x010000) <<  2)
         | (!!(r & 0x000800) <<  1) | (!!(r & 0x000400) <<  0);
    s = string(hex<2>(data), hex<2>(addr >> 16), "-", hex<4>(addr & 0xffff));
    s.transform("0123456789abcdef", "df4709156bc8a23e");
    return true;
  } else {
    return false;
  }
}

//========
//internal
//========

unsigned Cheat::mirror(unsigned addr) const {
  //$00-3f|80-bf:0000-1fff -> $7e:0000-1fff
  if((addr & 0x40e000) == 0x000000) return (0x7e0000 + (addr & 0x1fff));  
  return addr;
}

//=========
//CheatCode
//=========

bool CheatCode::operator=(string s) {
  addr.reset();
  data.reset();

  lstring list;
  list.split("+", s.replace(" ", ""));

  for(unsigned i = 0; i < list.size(); i++) {
    unsigned addr_;
    uint8 data_;
    Cheat::Type type_;
    if(Cheat::decode(list[i], addr_, data_, type_) == false) {
      addr.reset();
      data.reset();
      return false;
    }

    addr.append(addr_);
    data.append(data_);
  }

  return true;
}

CheatCode::CheatCode() {
  enabled = false;
}

}
