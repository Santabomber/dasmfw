/***************************************************************************
 * dasmfw -- Disassembler Framework                                        *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 2 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program; if not, write to the Free Software             *
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.               *
 ***************************************************************************/

/*****************************************************************************/
/* Disassembler.h : definition of the Disassembler abstract base class       */
/*****************************************************************************/

#ifndef __Disassembler_h_defined__
#define __Disassembler_h_defined__

#include "dasmfw.h"

/*****************************************************************************/
/* MemAttributeHandler : abstract base handler for memory attributes         */
/*****************************************************************************/

class MemAttributeHandler
  {
  public:
    MemAttributeHandler() { }
    virtual ~MemAttributeHandler() { }
    virtual bool AddMemory(addr_t addrStart = 0, addr_t memSize = 0, MemoryType memType = Code) = 0;
    virtual MemoryType GetMemType(addr_t addr) = 0;
    virtual void SetMemType(addr_t addr, MemoryType newType = Code) = 0;
    virtual bool IsCellUsed(addr_t addr) = 0;
    virtual void SetCellUsed(addr_t addr, bool bUsed = true) = 0;
    virtual MemAttribute::Type GetCellType(addr_t addr) = 0;
    virtual void SetCellType(addr_t addr, MemAttribute::Type newType) = 0;
    virtual int GetCellSize(addr_t addr) = 0;
    virtual void SetCellSize(addr_t addr, int newSize = 1) = 0;
    virtual MemAttribute::Display GetDisplay(addr_t addr) = 0;
    virtual void SetDisplay(addr_t addr, MemAttribute::Display newDisp = MemAttribute::DefaultDisplay) = 0;
    virtual bool GetBreakBefore(addr_t addr) = 0;
    virtual void SetBreakBefore(addr_t addr, bool bOn = true) = 0;
    virtual uint32_t GetDisassemblyFlags(addr_t addr, uint8_t mem, Label *plbl) = 0;
    // basic access
    virtual size_t size() = 0;
    virtual addr_t GetStart(int index) = 0;
    virtual size_t size(int index) = 0;
  };

/*****************************************************************************/
/* BasicMemAttributeHandler : memory attribute handler for basics            */
/*****************************************************************************/

enum BasicDataDisassemblyFlags
  {
  // lowest 8 bits are reserved for (cell size - 1); if the need arises for
  // larger cells, this has to be adjusted
  SHMF_DATA  = 1 << 8,                  /* Data byte (not Code)              */
  SHMF_RMB   = (SHMF_DATA << 1),        /* no defined contents               */
  SHMF_TXT   = (SHMF_RMB  << 1),        /* Textual display possible          */
  SHMF_BREAK = (SHMF_TXT << 1),         /* mandatory break before            */
  SHMF_NOTXT = (SHMF_BREAK << 1),       /* definitely no textual display     */

  SHMF_BasicSetMax = SHMF_NOTXT
  };

uint32_t GetBasicDisassemblyFlags(MemAttribute *pAttr, uint8_t mem, Label *plbl);
class BasicMemAttributeHandler : public MemAttributeHandler
{
  public:
    BasicMemAttributeHandler()
      { }
    virtual ~BasicMemAttributeHandler()
      { }

    virtual bool AddMemory(addr_t addrStart = 0, addr_t memSize = 0, MemoryType memType = Code)
      { return attr.AddMemory(addrStart, memSize, memType); }
    virtual MemoryType GetMemType(addr_t addr)
      { MemAttribute *pAttr = attr.getat(addr); return pAttr ? pAttr->GetMemType() : Untyped; }
    virtual void SetMemType(addr_t addr, MemoryType newType = Code)
      { MemAttribute *pAttr = attr.getat(addr); if (pAttr) pAttr->SetMemType(newType); }
    virtual bool IsCellUsed(addr_t addr)
      { MemAttribute *pAttr = attr.getat(addr); return pAttr ? pAttr->IsUsed() : false; }
    virtual void SetCellUsed(addr_t addr, bool bUsed = true)
      { MemAttribute *pAttr = attr.getat(addr); if (pAttr) pAttr->SetUsed(bUsed); }
    virtual MemAttribute::Type GetCellType(addr_t addr)
      { MemAttribute *pAttr = attr.getat(addr); return pAttr ? pAttr->GetCellType() : MemAttribute::CellUntyped; }
    virtual void SetCellType(addr_t addr, MemAttribute::Type newType)
      { MemAttribute *pAttr = attr.getat(addr); if (pAttr) pAttr->SetCellType(newType); }
    virtual int GetCellSize(addr_t addr)
      { MemAttribute *pAttr = attr.getat(addr); return pAttr ? pAttr->GetSize() : 0; }
    virtual void SetCellSize(addr_t addr, int newSize = 1)
      { MemAttribute *pAttr = attr.getat(addr); if (pAttr) pAttr->SetSize(newSize); }
    virtual MemAttribute::Display GetDisplay(addr_t addr)
      { MemAttribute *pAttr = attr.getat(addr); return pAttr ? pAttr->GetDisplay() : MemAttribute::CellUndisplayable; }
    virtual void SetDisplay(addr_t addr, MemAttribute::Display newDisp = MemAttribute::DefaultDisplay)
      { MemAttribute *pAttr = attr.getat(addr); if (pAttr) pAttr->SetDisplay(newDisp); }
    virtual bool GetBreakBefore(addr_t addr)
      { MemAttribute *pAttr = attr.getat(addr); return pAttr ? pAttr->GetBreakBefore() : false; }
    virtual void SetBreakBefore(addr_t addr, bool bOn = true)
      { MemAttribute *pAttr = attr.getat(addr); if (pAttr) pAttr->SetBreakBefore(bOn); }
    virtual uint32_t GetDisassemblyFlags(addr_t addr, uint8_t mem, Label *plbl)
      { return GetBasicDisassemblyFlags(attr.getat(addr), mem, plbl); }
    // basic access
    virtual size_t size() { return (size_t)attr.size(); }
    virtual addr_t GetStart(int index) { return attr[index].GetStart(); }
    virtual size_t size(int index) { return attr[index].size(); }

  protected:
    TMemoryArray <MemAttribute, MemoryType> attr;

};


/*****************************************************************************/
/* OpCode : a basic opcode definition                                        */
/*****************************************************************************/

struct OpCode
  {
  const char *mne;                      /* mnemonic                          */
  MemoryType memType;                   /* referenced memory type            */
  };

/*****************************************************************************/
/* Disassembler : abstract base class for a processor's disassembler         */
/*****************************************************************************/

class Disassembler
  {
  public:
    Disassembler();
    virtual ~Disassembler();

  // Endianness enumeration
  public:
    enum Endian
      {
      BigEndian,                        /* MSB first                         */
      LittleEndian                      /* LSB first                         */
      };
    enum Architecture
      {
      vonNeumann,                       /* Code and data share the same bus  */
      Harvard                           /* Code and data have separate bus   */
      };

  /* A processor has the following characteristics:
     .) number of busses.
        A von Neumann processor has at least one bus, which is used to
        transfer instruction and data bytes.
        A Harvard Architecture processor has at least two busses, one for the
        instruction bytes and one for the data bytes.
     .) bus width.
        Each bus transfers a number of bits at the same time.
     .) bus address range (lowest / highest address)
     .) Code / Data bits (informatory character only)

  */

  // Information about the processor's capabilities
  public:
    // return processor long name
    virtual string GetName() = 0;
    // return whether big- or little-endian
    virtual Endian GetEndianness() = 0;
    // return architecture type
    virtual Architecture GetArchitecture() { return vonNeumann; }  // most are vN

    // return number of busses
    virtual int GetBusCount() { return BaseBusTypes; }
    // return bus width
    virtual int GetBusWidth(int bus = BusCode) = 0;
    // return lowest address for a bus
    virtual addr_t GetLowestBusAddr(int bus = BusCode)
      { return 0; }
    // return highest address for a bus
    virtual addr_t GetHighestBusAddr(int bus = BusCode)
      { return (addr_t)(((addr_t)1 << GetBusWidth(bus)) - 1); }
    // return a bus name
    virtual string GetBusName(int bus = BusCode)
      {
      if (bus < 0 || bus >= GetBusCount()) return "";
      return busnames[bus];
      }
    // return a bus ID based on its name (or -1 if unknown)
    virtual int GetBusID(string busname);
    // return concatenated set of all bus names
    virtual string GetBusNames(char cDelim = '|')
      {
      string busses;
      for (int bus = 0; bus < GetBusCount(); bus++)
        {
        if (bus)
          busses += cDelim;
        busses += lowercase(GetBusName(bus));
        }
      return busses;
      }
    // return the default memory type for a bus (used in loading)
    virtual MemoryType GetDefaultMemoryType(int bus = BusCode)
      { return Code; }

    // return code bits
    virtual int GetCodeBits() = 0;
    // return data bits
    virtual int GetDataBits() = 0;

    // return code pointer size in bytes
    virtual int GetCodePtrSize() = 0;
    // return lowest possible code address
    virtual caddr_t GetLowestCodeAddr() { return 0; }
    // return highest possible code address
    virtual caddr_t GetHighestCodeAddr() = 0;
    // return data pointer size in bytes
    virtual int GetDataPtrSize() = 0;
    // return lowest possible data address
    virtual daddr_t GetLowestDataAddr() { return 0; }
    // return highest possible data address
    virtual daddr_t GetHighestDataAddr() = 0;

  // Setup disassembler after construction
    virtual bool Setup();

  // Generic options handler
  protected:
    typedef int (Disassembler::*PSetter)(string lname, string value);
    typedef string (Disassembler::*PGetter)(string lname);
    struct OptionHandler
      {
      string name;
      string help;
      PSetter setter;
      PGetter getter;
      };
    vector<OptionHandler *> options;
    bool AddOption(string name, string help, PSetter setter, PGetter getter);
    // 
    int DisassemblerSetOption(string lname, string value);
    string DisassemblerGetOption(string lname);
  public:
    int GetOptionCount() { return options.size(); }
    int FindOption(string name);
    string GetOptionName(int idx) { return options[idx]->name; }
    string GetOptionHelp(int idx) { return options[idx]->help; }
    int SetOption(int idx, string value) { return ((*this).*(options[idx]->setter))(options[idx]->name, value); }
    int SetOption(string name, string value);
    string GetOption(int idx) { return ((*this).*(options[idx]->getter))(options[idx]->name); }
    string GetOption(string name);
    // print disassembler-specific info file help
    virtual string InfoHelp() { return ""; }
    // global typed options
    addr_t GetBegin() { return begin; }
    addr_t GetEnd() { return end; }
    addr_t GetOffset() { return offset; }
    int GetBus(int bus) { return busorder[bus]; }

  // Memory access
  public:
    // Add memory area
    virtual bool AddMemory(addr_t addrStart = 0, addr_t memSize = 0, MemoryType memType = Code, uint8_t *contents = NULL, int bus = BusCode)
      {
      if (!memory[bus].AddMemory(addrStart, memSize, memType, contents))
        return false;
      return memattr[bus] && memattr[bus]->AddMemory(addrStart, memSize, memType);
      }
    template<class T> T inline getat(addr_t addr, int bus = BusCode)
      {
      T val;
      if (!getat(addr, (uint8_t *)&val, sizeof(val), bus))
        return 0;
      return val;
      }
    // Direct access to memory areas (... be careful!)
    int GetMemoryArrayCount(int bus = BusCode) { return memory[bus].size(); }
    TMemory<uint8_t, MemoryType> &GetMemoryArray(int index, int bus = BusCode) { return memory[bus].at(index); }
    int GetMemAttrArrayCount(int bus = BusCode) { return memattr[bus] ? memattr[bus]->size() : 0; }
    MemAttributeHandler *GetMemoryAttributeHandler(int bus = BusCode) { return memattr[bus]; }
    // find memory area index for a given address
    addr_t GetMemIndex(addr_t addr, int bus = BusCode)
      { return memory[bus].GetMemIndex(addr); }
    // get next loaded memory address
    addr_t GetNextAddr(addr_t addr, int bus = BusCode)
      {
      addr_t nextAddr = addr + 1;
      while (!IsCellUsed(nextAddr, bus))
        {
        uint8_t *pNext = getat(nextAddr, bus);
        if (!pNext)
          {
          for (int i = 0; i < GetMemoryArrayCount(); i++)
            {
            addr_t nextStart = GetMemoryArray(i, bus).GetStart();
            if (nextStart > nextAddr)
              {
              pNext = getat(nextStart, bus);
              nextAddr = nextStart;
              break;
              }
            }
          if (!pNext)
            return NO_ADDRESS;
          }
        else
          nextAddr++;
        }
      return nextAddr;
      }

    // Get/Set integer memory as defined by cell type / size
    addr_t GetTypedInt(addr_t addr, int bus = BusCode);
    void SetTypedInt(addr_t value, addr_t addr, int bus = BusCode);
    // Get/Set memory byte
    int8_t GetSByte(addr_t addr, int bus = BusCode)
      { return (int8_t)*getat(addr, bus); }
    uint8_t GetUByte(addr_t addr, int bus = BusCode)
      { uint8_t *pb = getat(addr, bus); return pb ? *pb : 0; }
    bool SetSByte(addr_t addr, int8_t val, int bus = BusCode)
      { return setat(addr, (uint8_t)val, bus); }
    bool SetUByte(addr_t addr, uint8_t val, int bus = BusCode)
      { return setat(addr, val, bus); }
    // Get/Set memory word
    int16_t GetSWord(addr_t addr, int bus = BusCode)
      { return getat<int16_t>(addr, bus); }
    uint16_t GetUWord(addr_t addr, int bus = BusCode)
      { return getat<uint16_t>(addr, bus); }
    bool SetSWord(addr_t addr, int16_t val, int bus = BusCode)
      { return setat(addr, (uint8_t *)&val, sizeof(val), bus); }
    bool SetUWord(addr_t addr, uint16_t val, int bus = BusCode)
      { return setat(addr, (uint8_t *)&val, sizeof(val), bus); }
    // Get/Set memory dword
    int32_t GetSDWord(addr_t addr, int bus = BusCode)
      { return getat<int32_t>(addr, bus); }
    uint32_t GetUDWord(addr_t addr, int bus = BusCode)
      { return getat<uint32_t>(addr, bus); }
    bool SetSDWord(addr_t addr, int32_t val, int bus = BusCode)
      { return setat(addr, (uint8_t *)&val, sizeof(val), bus); }
    bool SetUDWord(addr_t addr, uint32_t val, int bus = BusCode)
      { return setat(addr, (uint8_t *)&val, sizeof(val), bus); }
    // Get/Set memory qword
    int64_t GetSQWord(addr_t addr, int bus = BusCode)
      { return getat<int64_t>(addr, bus); }
    uint64_t GetUQWord(addr_t addr, int bus = BusCode)
      { return getat<uint64_t>(addr, bus); }
    bool SetSQWord(addr_t addr, int64_t val, int bus = BusCode)
      { return setat(addr, (uint8_t *)&val, sizeof(val), bus); }
    bool SetUQWord(addr_t addr, uint64_t val, int bus = BusCode)
      { return setat(addr, (uint8_t *)&val, sizeof(val), bus); }
    // Get/Set 32-bit floating-point IEEE 854 value
    float GetFloat(addr_t addr, int bus = BusCode)
      { return getat<float>(addr, bus); }
    bool SetFloat(addr_t addr, float val, int bus = BusCode)
      { return setat(addr, (uint8_t *)&val, sizeof(val), bus); }
    // Get/Set 64-bit floating-point IEEE 854 value
    double GetDouble(addr_t addr, int bus = BusCode)
      { return getat<double>(addr, bus); }
    bool SetDouble(addr_t addr, double val, int bus = BusCode)
      { return setat(addr, (uint8_t *)&val, sizeof(val), bus); }
    // Get/set delimited ASCII string
    string GetString(addr_t addr, char cTerm = '\0', int bus = BusCode)
      {
      string s;
      char c;
      while ((c = (char)getat(addr++)) != cTerm)
        s += c;
      return s;
      }
    bool SetString(addr_t addr, string s, char cTerm = '\0', int bus = BusCode)
      {
      if (setat(addr, (uint8_t *)s.c_str(), s.size(), bus))
        return setat(addr + s.size(), (uint8_t)cTerm, bus);
      return false;
      }
    // in theory, strings with leading length byte/word might come up at some time
    // we'll deal with it in time.

    // Get/Set cell information
    MemoryType GetMemType(addr_t addr, int bus = BusCode)
      { return memattr[bus] ? memattr[bus]->GetMemType(addr) : Untyped; }
    void SetMemType(addr_t addr, MemoryType newType = Code, int bus = BusCode)
      { if (memattr[bus]) memattr[bus]->SetMemType(addr, newType); }
    bool IsCellUsed(addr_t addr, int bus = BusCode)
      { return memattr[bus] ? memattr[bus]->IsCellUsed(addr) : false; }
    void SetCellUsed(addr_t addr, bool bUsed = true, int bus = BusCode)
      { if (memattr[bus]) memattr[bus]->SetCellUsed(addr, bUsed); }
    MemAttribute::Type GetCellType(addr_t addr, int bus = BusCode)
      { return memattr[bus] ? memattr[bus]->GetCellType(addr) : MemAttribute::CellUntyped; }
    void SetCellType(addr_t addr, MemAttribute::Type newType, int bus = BusCode)
      { if (memattr[bus]) memattr[bus]->SetCellType(addr, newType); }
    int GetCellSize(addr_t addr, int bus = BusCode)
      { return memattr[bus] ? memattr[bus]->GetCellSize(addr) : 0; }
    void SetCellSize(addr_t addr, int newSize = 1, int bus = BusCode)
      { if (memattr[bus]) memattr[bus]->SetCellSize(addr, newSize); }
    MemAttribute::Display GetDisplay(addr_t addr, int bus = BusCode)
      { return memattr[bus] ? memattr[bus]->GetDisplay(addr) : MemAttribute::CellUndisplayable; }
    void SetDisplay(addr_t addr, MemAttribute::Display newDisp = MemAttribute::DefaultDisplay, int bus = BusCode)
      { if (memattr[bus]) memattr[bus]->SetDisplay(addr, newDisp); }
    bool GetBreakBefore(addr_t addr, int bus = BusCode)
      { return memattr[bus] ? memattr[bus]->GetBreakBefore(addr) : false; }
    void SetBreakBefore(addr_t addr, bool bOn = true, int bus = BusCode)
      { if (memattr[bus]) memattr[bus]->SetBreakBefore(addr, bOn); }
    // get/set default cell display format
    MemAttribute::Display GetDisplay() { return defaultDisplay; }
    void SetDisplay(MemAttribute::Display newDisp) { defaultDisplay = newDisp; }
    // Get Flags for disassembly of data areas
    uint32_t GetDisassemblyFlags(addr_t addr, int bus = BusCode)
      {
      uint8_t *mem = getat(addr, bus);
      if (!mem) return 0;
      // NB: this uses the LAST label for that address
      Label *plbl = FindLabel(addr, Untyped, bus);
      return memattr[bus] ? memattr[bus]->GetDisassemblyFlags(addr, *mem, plbl) : 0;
      }
    void SetInvalidInstruction(addr_t addr, int newSize = 1, int bus = BusCode)
      {
      // this can be called during disassembler parsing to mark an invalid instruction
      SetMemType(addr, Const);
      SetCellSize(addr, newSize);
      SetCellType(addr, MemAttribute::CellUntyped);
      }

    // convenience functionality for the above
    bool IsCode(addr_t addr, int bus = BusCode) { return GetMemType(addr, bus) == Code; }
    bool IsData(addr_t addr, int bus = BusCode) { return GetMemType(addr, bus) == Data; }
    bool IsConst(addr_t addr, int bus = BusCode) { return GetMemType(addr, bus) == Const; }
    bool IsBss(addr_t addr, int bus = BusCode) { return GetMemType(addr, bus) == Bss; }
    bool IsBinary(addr_t addr, int bus = BusCode) { return GetDisplay(addr, bus) == MemAttribute::Binary; }
    bool IsChar(addr_t addr, int bus = BusCode) { return GetDisplay(addr, bus) == MemAttribute::Char; }
    bool IsOctal(addr_t addr, int bus = BusCode) { return GetDisplay(addr, bus) == MemAttribute::Octal; }
    bool IsDecimal(addr_t addr, int bus = BusCode) { return GetDisplay(addr, bus) == MemAttribute::Decimal; }
    bool IsHex(addr_t addr, int bus = BusCode) { MemAttribute::Display disp = GetDisplay(addr, bus); return disp == MemAttribute::DefaultDisplay || disp == MemAttribute::Hex; }
    bool IsSigned(addr_t addr, int bus = BusCode) { return GetCellType(addr, bus) == MemAttribute::SignedInt; }
    bool IsUnsigned(addr_t addr, int bus = BusCode) { return GetCellType(addr, bus) == MemAttribute::UnsignedInt; }
    bool IsFloat(addr_t addr, int bus = BusCode) { return GetCellType(addr, bus) == MemAttribute::Float; }

  // Relative handling
  public:
    bool AddRelative(addr_t addr, addr_t relsize = 1,  addr_t *contents = NULL, int bus = BusCode)
      { return Relatives[bus].AddMemory(addr, relsize, 0, contents); }
    addr_t GetRelative(addr_t addr, int bus = BusCode)
      { addr_t *paddr = Relatives[bus].getat(addr); return paddr ? *paddr : 0; }
    void SetRelative(addr_t addr, addr_t rel, int bus = BusCode)
      { addr_t *paddr = Relatives[bus].getat(addr); if (paddr) *paddr = rel; }

  // Phase handling
  public:
    bool AddPhase(addr_t addr, addr_t phaseSize = 1, addr_t phase = 0, int bus = BusCode)
      { return Phases[bus].AddMemory(addr, phaseSize, phase); }
    TMemory<addr_t, addr_t> *FindPhase(addr_t addr, int bus = BusCode)
      { return Phases[bus].FindMem(addr); }
    addr_t GetPhase(addr_t addr, int bus = BusCode)
      { TMemory<addr_t, addr_t> *pArea = FindPhase(addr, bus); return pArea ? pArea->GetType() : NO_ADDRESS; }
    void SetPhase(addr_t addr, addr_t phase = NO_ADDRESS, int bus = BusCode)
      { addr_t *paddr = Phases[bus].getat(addr); if (paddr) *paddr = phase; }
    protected:
      addr_t PhaseInner(addr_t value, addr_t addr, int bus = BusCode)
        {
        // "phase" an address if it's inside the current range
        TMemory<addr_t, addr_t> *pArea = Phases[bus].FindMem(addr);
        if (pArea)
          {
          addr_t phStart = pArea->GetType();
          addr_t phEnd = phStart + pArea->size() - 1;
          addr_t offset = pArea->at(addr);
          if (value >= phStart && value <= phEnd || offset != NO_ADDRESS)
            {
            addr_t aStart = pArea->GetStart();
            if (offset == NO_ADDRESS)
              offset = phStart - aStart;
            else if (offset == DEFAULT_ADDRESS)
              offset = 0;
            else
              offset -= aStart;
            value -= offset;
            }
          }
        return value;
        }
      addr_t DephaseOuter(addr_t value, addr_t addr, int bus = BusCode)
        {
        // "de-phase" an address if it's outside the current range
        TMemory<addr_t, addr_t> *pArea = Phases[bus].FindMem(addr);
        if (pArea)
          {
          addr_t aStart = pArea->GetStart();
          addr_t aEnd = pArea->GetEnd();
          addr_t offset = pArea->at(addr);
          if (value < aStart || value > aEnd || offset != NO_ADDRESS)
            {
            addr_t phStart = pArea->GetType();
            if (offset == NO_ADDRESS)
              offset = aStart - phStart;
            else if (offset == DEFAULT_ADDRESS)
              offset = 0;
            else
              offset -= phStart;
            value -= offset;
            }
          }
        return value;
        }

  // Label handling
  public:
    bool AddLabel(addr_t addr, MemoryType memType = Code, string sLabel = "", bool bUsed = false, int bus = BusCode);
    Label *GetFirstLabel(addr_t addr, LabelArray::iterator &it, MemoryType memType = Untyped, int bus = BusCode)
      { return Labels[bus].GetFirst(addr, it, memType); }
    Label *GetNextLabel(addr_t addr, LabelArray::iterator &it, MemoryType memType = Untyped, int bus = BusCode)
      { return Labels[bus].GetNext(addr, it, memType); }
    Label *FindLabel(addr_t addr, MemoryType memType = Untyped, int bus = BusCode);
    Label *FindPrevNamedLabel(addr_t addr, MemoryType memType = Untyped, int bus = BusCode);
    void RemoveLabel(addr_t addr, MemoryType memType = Untyped, int bus = BusCode)
      {
      LabelArray::iterator p = Labels[bus].find(addr, memType);
      if (p != Labels[bus].end()) Labels[bus].erase(p);
      }
    int GetLabelCount(int bus = BusCode) { return Labels[bus].size(); }
    Label *LabelAt(int index, int bus = BusCode) { return (Label *)Labels[bus].at(index); }
    void RemoveLabelAt(int index, int bus = BusCode) { Labels[bus].erase(Labels[bus].begin() + index); }
    virtual bool ResolveLabels(int bus = BusCode);
    // convenience functionality for the above
    string GetLabel(addr_t addr, MemoryType memType = Untyped, int bus = BusCode)
      {
      // NB: this returns the LAST label for this address and type!
      Label *p = FindLabel(addr, memType, bus);
      return p ? p->GetText() : "";
      }
    bool IsLabel(addr_t addr, int bus = BusCode)
      {
      // returns whether ANY kind of label is set here
      return !!FindLabel(addr, Untyped, bus);
      }
    bool IsCLabel(addr_t addr, int bus = BusCode)
      {
      LabelArray::iterator it;
      Label *pLabel = GetFirstLabel(addr, it, Untyped, bus);
      while (pLabel)
        {
        if (pLabel->IsCode()) return true;
        pLabel = GetNextLabel(addr, it, Untyped, bus);
        }
      return false;
      }
    bool IsDLabel(addr_t addr, int bus = BusCode)
      {
      LabelArray::iterator it;
      Label *pLabel = GetFirstLabel(addr, it, Untyped, bus);
      while (pLabel)
        {
        if (pLabel->IsData()) return true;
        pLabel = GetNextLabel(addr, it, Untyped, bus);
        }
      return false;
      }
    bool IsDefLabel(addr_t addr, int bus = BusCode)
      {
      LabelArray::iterator it;
      Label *pLabel = GetFirstLabel(addr, it, Untyped, bus);
      while (pLabel)
        {
        if (pLabel->IsConst()) return true;
        pLabel = GetNextLabel(addr, it, Untyped, bus);
        }
      return false;
      }
    bool IsIOLabel(addr_t addr, int bus = BusCode)
      {
      LabelArray::iterator it;
      Label *pLabel = GetFirstLabel(addr, it, Untyped, bus);
      while (pLabel)
        {
        if (pLabel->IsIOPort()) return true;
        pLabel = GetNextLabel(addr, it, Untyped, bus);
        }
      return false;
      }
    void SetLabelUsed(addr_t addr, MemoryType memType = Code, int bus = BusCode)
      {
      LabelArray::iterator it;
      Label *pLabel = GetFirstLabel(addr, it, memType, bus);
      while (pLabel)
        {
        // "Const" is a DefLabel, so only match it if Const is requested
        if (memType == Const || !pLabel->IsConst())
          pLabel->SetUsed();            /* mark it as used                   */
        pLabel = GetNextLabel(addr, it, memType, bus);
        }
      }

  // Definition Label handling
    public:
      bool AddDefLabel(addr_t addr, string sLabel = "", string sDefinition = "", MemoryType memType = Code, int bus = BusCode)
        {
        if (!DefLabels[bus].Find(sLabel))
          DefLabels[bus].insert(new DefLabel(DefLabels[bus].size(),
                                             memType,
                                             sLabel,
                                             sDefinition));
        return true;
        }
      DefLabel *FindDefLabel(addr_t addr, int bus = BusCode)
        { return DefLabels[bus].Find(addr); }
    int GetDefLabelCount(int bus = BusCode) { return DefLabels[bus].size(); }
    DefLabel *DefLabelAt(int index, int bus = BusCode) { return (DefLabel *)DefLabels[bus].at(index); }

  // File handling
  public:
    // load a code file; interleave can be >1 for interleaved Low/High EPROM pairs, for example
    bool Load(string filename, string &sLoadType, int interleave = 1, int bus = BusCode);
    // process an info file line
    virtual bool ProcessInfo(string key, string value, addr_t &from, addr_t &to, vector<TMemoryArray<addr_t>> &remaps, bool bProcInfo = true, int bus = BusCode, int tgtbus = BusCode) { return false; }

  // the real disassembler activities
  protected:
    // parse data area for labels
    virtual addr_t ParseData(addr_t addr, int bus = BusCode) = 0;
    // parse instruction at given memory address for labels
    virtual addr_t ParseCode(addr_t addr, int bus = BusCode) = 0;
    // disassemble data area at given memory address
    virtual addr_t DisassembleData(addr_t addr, addr_t end, uint32_t flags, string &smnemo, string &sparm, int maxparmlen, int bus = BusCode) = 0;
    // disassemble instruction at given memory address
    virtual addr_t DisassembleCode(addr_t addr, string &smnemo, string &sparm, int bus = BusCode) = 0;
  // globally accessible dispatchers for the above
  public:
    // Initialize parsing
    virtual bool InitParse(int bus = BusCode)
      {
      if (bus == BusCode && load != NO_ADDRESS && bLoadLabel)
        AddLabel(load, Code, "", true, bus);
      return true;
      }
    // Parse memory area for labels
    addr_t Parse(addr_t addr, int bus = BusCode)
      { return IsCode(addr, bus) ? ParseCode(addr, bus) : ParseData(addr, bus); }
    // Disassemble a line in the memory area
    addr_t Disassemble(addr_t addr, string &smnemo, string &sparm, int maxparmlen, int bus = BusCode)
      {
      if (IsCode(addr, bus)) return DisassembleCode(addr, smnemo, sparm, bus);
      uint32_t flags = 0;
      addr_t end = GetConsecutiveData(addr, flags, maxparmlen, bus);
      return DisassembleData(addr, end, flags, smnemo, sparm, maxparmlen, bus);
      }
    // pass back correct mnemonic and parameters for a label
    virtual bool DisassembleLabel(Label *label, string &slabel, string &smnemo, string &sparm, int bus = BusCode)
      { return false; } // no changes in base implementation
    // pass back correct mnemonic and parameters for a DefLabel
    virtual bool DisassembleDefLabel(DefLabel *label, string &slabel, string &smnemo, string &sparm, int bus = BusCode)
      { return false; } // no changes in base implementation
    // pass back disassembler-specific state changes before/after a disassembly line
    struct LineChange
      {
      string label;
      string oper;
      string opnds;
      };
    virtual bool DisassembleChanges(addr_t addr, addr_t prevaddr, addr_t prevsz, bool bAfterLine, vector<LineChange> &changes, int bus = BusCode)
      { return changes.size() != 0; } // no additional changes in base implementation
    // create hex / ASCII representation of the current area
    virtual bool DisassembleHexAsc(addr_t addr, addr_t len, addr_t max, string &sHex, string &sAsc, int bus = BusCode)
      {
      sAsc = "\'";
      addr_t i;
      for (i = 0; i < len; i++)
        {
        uint8_t c = GetUByte(addr + i, bus);
        sHex += sformat("%02X ", c);
        sAsc += (isprint(c)) ? c : '.'; 
        }
      sAsc += '\'';
      for (; i < max; i++)
        {
        sHex += "   ";
        sAsc += ' ';
        }
      return true;
      }

  protected:
    // must not be called from constructor!
    virtual MemAttributeHandler *CreateAttributeHandler() { return new BasicMemAttributeHandler; }
    //
    TMemory<uint8_t, MemoryType> *FindMem(addr_t addr, int bus = BusCode)
      { return memory[bus].FindMem(addr); }
    // fetch byte at a given address
    uint8_t *getat(addr_t addr, int bus = BusCode)
      { return memory[bus].getat(addr); }
    // fetch multiple bytes with endianness correction
    bool getat(addr_t addr, uint8_t *val, addr_t len, int bus = BusCode) 
      { return memory[bus].getat(addr, val, len, (GetEndianness() != prgEndian)); }
    // write byte at a given address
    bool setat(addr_t addr, uint8_t val, int bus = BusCode) 
      { return memory[bus].setat(addr, val); }
    // write multiple bytes with endianness correction
    bool setat(addr_t addr, uint8_t *val, addr_t len, int bus = BusCode) 
      { return memory[bus].setat(addr, val, len, (GetEndianness() != prgEndian)); }

    // load opened file (overridable for specific file types)
    virtual bool LoadFile(string filename, FILE *f, string &sLoadType, int interleave = 1, int bus = BusCode);
    bool LoadIntelHex(string filename, FILE *f, string &sLoadType, int interleave = 1, int bus = BusCode);
    bool LoadMotorolaHex(string filename, FILE *f, string &sLoadType, int interleave = 1, int bus = BusCode);
    bool LoadBinary(string filename, FILE *f, string &sLoadType, int interleave = 1, int bus = BusCode);

    // calculate bits needed for an address
    int CalcBitsForHighestAddr(addr_t addr)
      {
      int bits = sizeof(addr) * 8;
      // this relies on a layout where the highest address
      // has all bits set (0xff, 0xffff, 0x3ffff, ...)
      addr++;
      for (int i = bits - 1; i >= 0; i--)
        if (addr & (1 << i))
          {
          bits = i;
          break;
          }
      return bits;
      }
    virtual void RecalcBusBits(int bus = BusCode)
      { busbits[bus] = CalcBitsForHighestAddr(GetHighestBusAddr(bus)); }
    // calculate consecutive data range (i.e., same type for all)
    addr_t GetConsecutiveData(addr_t addr, uint32_t &flags, int maxparmlen, int bus = BusCode);

  public:
    // return address bits for a specific bus
    int BusAddressBits(int bus = BusCode) { return busbits[bus]; }
    // convert a string to an integer number, (dis)assembler-specific
    virtual bool String2Number(string s, addr_t &value)
      {
      int base = pbase;
      s = trim(s);
      // Only thing that should always work...
      // C-style number strings should be universal
      string sFmt = s.substr(0, 2);
      if (sFmt == "0x")
        {
        base = 16;
        s = s.substr(2);
        }
      // octal (leading 0), however, isn't.
      // So interpret any pure number as having the default base.
      char const *start = s.c_str();
      char *end = NULL;
      value = strtoul(start, &end, base);
#if 0
      return (start != end);
#else
      char c = end ? tolower(*end) : 0;
      return start != end && !isalnum(c);
      // return end && !*end;
#endif
      }
    virtual int String2Range(string s, addr_t &from, addr_t &to)
      {
      from = to = NO_ADDRESS;
      string::size_type midx = s.find('-');
      if (midx == 0)
        {
        midx = s.substr(1).find('-');
        if (midx != s.npos) midx++;
        }
      if (midx != s.npos)
        return (int)String2Number(s.substr(0, midx), from) +
               (int)String2Number(s.substr(midx + 1), to);
      else
        return (int)String2Number(s, from);
      }
    // convert a string to a floating-point number, (dis)assembler-specific
    virtual bool String2Double(string s, double &value)
      {
      // Only thing that should always work...
      value = 0.;
      return (sscanf(s.c_str(), "%lf", &value) == 1);
      }
    // convert a number to a string
    virtual string Number2String(addr_t value, int nDigits, addr_t addr, int bus = BusCode)
      {
#if 0
      MemoryType memType = GetMemType(addr);
      MemAttribute::Display disp;
      bool bSigned = false;
      if (memType == MemAttribute::CellUntyped)
        disp = defaultDisplay;
      else
        disp = GetDisplay(addr);
      if (disp == MemAttribute::DefaultDisplay)
        disp = defaultDisplay;
      if (disp == MemAttribute::Hex)
        return sformat("0x%0*x", nDigits, value);
      else
#endif
      // Only thing that should always work...
      return sformat(IsSigned(addr, bus) ? "%d" : "%u", value);
      }
    virtual string SignedNumber2String(saddr_t value, int nDigits, addr_t addr, int bus = BusCode)
      {
      string s;
      // specialization for things that have to be signed in any case
      if (value < 0)
        {
        s = "-";
        value = -value;
        }
      return s + Number2String((addr_t)value, nDigits, addr, bus);
      }
    virtual string Address2String(addr_t addr, int bus = BusCode)
      { return sformat("%d", addr); }
    virtual string Label2String(addr_t value, int nDigits, bool bUseLabel, addr_t addr, int bus = BusCode);
    virtual string DefLabel2String(addr_t value, int nDigits, addr_t addr, int bus = BusCode);
    virtual string AutoLabel2String(addr_t addr, bool bCode, int bus = BusCode);
    // generate text for an unnamed label
    virtual string UnnamedLabel(addr_t addr, bool bCode, int bus = BusCode)
      {
      if (bAutoLabel)
        {
        // auto-label based on previous constant, if possible
        string s = AutoLabel2String(addr, bCode, bus);
        if (s.size())
          return s;
        }

      const char *cType = bCode ? "Z" : "M";
      int bits = busbits[bus];
      return sformat("%s%0*X", cType, (bits + 3) / 4, addr);
      }

  protected:
    static const Endian prgEndian;
    // For all [BusTypes] arrays below:
    // [0]: instruction bus; [1]: data bus, if separate; [2]: I/O bus, if separate
    vector<string> busnames;
    vector<MemoryArray> memory;
    vector<MemAttributeHandler *> memattr;
    vector<LabelArray> Labels;
    vector<DefLabelArray> DefLabels;
    vector<TMemoryArray<addr_t>> Relatives;
    vector<TMemoryArray<addr_t, addr_t>> Phases;
    vector<int> busorder;
    vector<int> busbits;

    // begin / end / load (i.e., entry point) address
    addr_t begin, end, load, offset;
    // flag whether to honor load address
    bool bLoadLabel;
    // flag whether to set system vector labels
    bool bSetSysVec;
    // flag whether to allow multiple labels for an address
    bool bMultiLabel;
    // flag whether to auto-generate labels based on previous defined label
    bool bAutoLabel;
    // default display format
    MemAttribute::Display defaultDisplay;
    // disassembler-specific comment start character
    string commentStart;
    // disassembler-specific label delimiter
    string labelDelim;
    // parsing radix (default 10)
    int pbase;
    uint32_t disassemblyFlagMask;

    vector<OpCode> mnemo;

  };

addr_t GetHex(FILE *f, int nChars, uint8_t *pchks = NULL);

#endif // __Disassembler_h_defined__
