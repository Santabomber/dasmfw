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
/* dasmfw.cpp : main program for the DisASseMbler FrameWork                  */
/*****************************************************************************/

#include "dasmfw.h"
#include "Disassembler.h"
#include "Label.h"
#include "Memory.h"

/*****************************************************************************/
/* Global Data                                                               */
/*****************************************************************************/

static struct DisassemblerCreators      /* structure for dasm creators:      */
  {
  std::string name;                     /* code name of the disassembler     */
  Disassembler *(*Factory)();           /* factory to create it              */
  } *Disassemblers = NULL;
static int nRegDisassemblers = 0;       /* # registered disassemblers        */
static int nAllocDisassemblers = 0;     /* # allocated disassembler structs  */

/*****************************************************************************/
/* RegisterDisassembler : registers a processor handler                         */
/*****************************************************************************/

bool RegisterDisassembler(std::string name, Disassembler *(*CreateDisassembler)())
{
if (nRegDisassemblers + 1 > nAllocDisassemblers)
  {
  DisassemblerCreators *pNew;
  try
    {
    pNew = new DisassemblerCreators[nRegDisassemblers + 10];
    if (!pNew)  // deal with very old style allocator
      return false;
    }
  catch(...)
    {
    return false;
    }
  for (int i = 0; i < nRegDisassemblers; i++)
    {
    pNew[i].name = Disassemblers[i].name;
    pNew[i].Factory = Disassemblers[i].Factory;
    }
  if (Disassemblers) delete[] Disassemblers;
  Disassemblers = pNew;
  }

// simple insertion sort will do
for (int i = 0; i < nRegDisassemblers; i++)
  {
  if (Disassemblers[i].name > name)
    {
    for (int j = ++nRegDisassemblers; j > i; j--)
      {
      Disassemblers[j].name = Disassemblers[j - 1].name;
      Disassemblers[j].Factory = Disassemblers[j - 1].Factory;
      }
    Disassemblers[i].name = name;
    Disassemblers[i].Factory = CreateDisassembler;
    return true;
    }
  }
Disassemblers[nRegDisassemblers].name = name;
Disassemblers[nRegDisassemblers++].Factory = CreateDisassembler;
return true;
}

/*****************************************************************************/
/* CreateDisassembler : creates a disassembler with a given code name        */
/*****************************************************************************/

Disassembler *CreateDisassembler(std::string name, int *pidx = NULL)
{
for (int da = 0; da < nRegDisassemblers; da++)
  if (Disassemblers[da].name == name)
    {
    if (pidx) *pidx = da;
    return Disassemblers[da].Factory();
    }
return NULL;
}

/*****************************************************************************/
/* ListDisassemblers : lists out all known disassemblers                     */
/*****************************************************************************/

static void ListDisassemblers()
{
printf("\nRegistered disassemblers:\n"
       "%-8s %-16s %s\n"
       "--------------------------------------------------------------\n",
       "Code", "Instruction Set", "Attributes");
for (int da = 0; da < nRegDisassemblers; da++)
  {
  Disassembler *pDasm = Disassemblers[da].Factory();
  printf("%-8s %-16s (%d/%d bits data/code, %s-endian",
         Disassemblers[da].name.c_str(),
         pDasm->GetName().c_str(),
         pDasm->GetDataBits(),
         pDasm->GetCodeBits(),
         pDasm->GetEndianness() == Disassembler::BigEndian ? "big" : "little");
  if (pDasm->GetArchitecture() == Disassembler::Harvard)
    printf(", separate data bus");
  printf(")\n");
  delete pDasm;
  }
}

/*****************************************************************************/
/* sformat : sprintf for std::string                                         */
/*****************************************************************************/

std::string sformat(const std::string fmt_str, ...)
{
int final_n, n = ((int)fmt_str.size()) * 2;
std::vector<char> formatted;
va_list ap;
while (1)
  {
  formatted.resize(n);
  va_start(ap, fmt_str);
  final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
  va_end(ap);
  if (final_n < 0 || final_n >= n)
    n += abs(final_n - n + 1);
  else
    break;
  }
return std::string(&formatted[0]);
}

/*****************************************************************************/
/* lowercase : create lowercase copy of a string                             */
/*****************************************************************************/

std::string lowercase(std::string s)
{
std::string sout;
// primitive ASCII implementation
for (std::string::size_type i = 0; i < s.size(); i++)
  sout += tolower(s[i]);
return sout;
}

/*****************************************************************************/
/* uppercase : create uppercase copy of a string                             */
/*****************************************************************************/

std::string uppercase(std::string s)
{
std::string sout;
// primitive ASCII implementation
for (std::string::size_type i = 0; i < s.size(); i++)
  sout += toupper(s[i]);
return sout;
}

/*****************************************************************************/
/* trim : remove leading and trailing blanks from string                     */
/*****************************************************************************/

std::string trim(std::string s)
{
if (s.empty()) return s;
std::string::size_type from = s.find_first_not_of(" ");
if (from == s.npos)
  return "";
return s.substr(from, s.find_last_not_of(" ") - from + 1);
}

/*****************************************************************************/
/* main : main program function                                              */
/*****************************************************************************/

int main(int argc, char* argv[])
{
Application app(argc, argv);
return app.Run();
}



/*===========================================================================*/
/* Application class members                                                 */
/*===========================================================================*/

/*****************************************************************************/
/* Application : constructor                                                 */
/*****************************************************************************/

Application::Application(int argc, char* argv[])
  : argc(argc), argv(argv)

{
pDasm = NULL;                           /* selected disassembler             */
iDasm = -1;                             /* index of selected disassembler    */
bInfoBus = false;                       /* false = code, true = data         */
showHex = true;                         /* flag for hex data display         */
showAddr = true;                        /* flag for address display          */
#if RB_VARIANT
showAsc = false;                        /* flag for ASCII content display    */
#else
showAsc = true;                         /* flag for ASCII content display    */
#endif
out = stdout;
comments[0][0].SetMultipleDefs();       /* allow multi-line comments / lines */
comments[0][1].SetMultipleDefs();
comments[1][0].SetMultipleDefs();
comments[1][1].SetMultipleDefs();
lcomments[0].SetMultipleDefs();
lcomments[1].SetMultipleDefs();

// pre-load matching disassembler, if possible
sDasmName = argv[0];
std::string::size_type idx = sDasmName.find_last_of("\\/");
if (idx != sDasmName.npos)
  sDasmName = sDasmName.substr(idx + 1);
idx = sDasmName.rfind('.');
if (idx != sDasmName.npos)
  sDasmName = sDasmName.substr(0, idx);
// if the name is "dasm"+code, use this disassembler
if (lowercase(sDasmName.substr(0, 4)) == "dasm")
  pDasm = CreateDisassembler(sDasmName.substr(4), &iDasm);

printf("%s: %s\n", sDasmName.c_str(), "Disassembler Framework V" DASMFW_VERSION);
}

/*****************************************************************************/
/* ~Application : destructor                                                 */
/*****************************************************************************/

Application::~Application()
{
}

/*****************************************************************************/
/* Run : the application's main functionality                                */
/*****************************************************************************/

int Application::Run()
{
if (argc < 2)                           /* if no arguments given, give help  */
  return Help();                        /* and exit                          */

std::string defnfo[2];                  /* global / user default info files  */
#ifdef _MSC_VER
// Microsoft C - guaranteed Windows environment
char const *pHomePath = getenv("USERPROFILE");
defnfo[0] = pHomePath ?
    std::string(pHomePath) + "\\.dasmfw\\" + sDasmName + ".nfo" :
    "";
#else
char const *pHomePath = getenv("HOME");
defnfo[0] = pHomePath ?
    std::string(pHomePath) + "/.dasmfw/" + sDasmName + ".nfo" :
    "";
#endif
defnfo[1] = sDasmName + ".nfo";

int i;
                                        /* first, just search for "dasm"     */
for (i = 0; i < _countof(defnfo); i++)
  if (!defnfo[i].empty())
    ParseOption("info", defnfo[i], true, false);
ParseOptions(argc, argv, true);
if (!pDasm)
  return Help();
                                        /* then parse all other options      */
ParseOptions(argc, argv, false);

int nInterleave = 1;
for (i = 0;                             /* load file(s) given on commandline */
     i < (int)saFNames.size();          /* and parsed from info files        */
     i++)
  {                                     /* and load-relevant option settings */
  if (saFNames[i].substr(0, 7) == "-begin:")
    ParseOption("begin", saFNames[i].substr(7));
  else if (saFNames[i].substr(0, 5) == "-end:")
    ParseOption("end", saFNames[i].substr(5));
  else if (saFNames[i].substr(0, 8) == "-offset:")
    ParseOption("offset", saFNames[i].substr(8));
  else if (saFNames[i].substr(0, 12) == "-interleave:")
    nInterleave = atoi(saFNames[i].substr(12).c_str());
  else
    {
    std::string sLoadType;
    bool bOK = pDasm->Load(saFNames[i], sLoadType, nInterleave);
    saFNames[i] = sformat("%soaded: %s file \"%s\"",
                          bOK ? "L" : "NOT l",
                          sLoadType.c_str(),
                          saFNames[i].c_str());
    if (nInterleave != 1)
      saFNames[i] += sformat(" (interleave=%d)", nInterleave);
    printf("%s\n", saFNames[i].c_str());
    }
  }
                                        /* then process all info files       */
for (i = 0; i < _countof(defnfo); i++)
  if (!defnfo[i].empty())
    {
    bool bOK = LoadInfo(defnfo[i]);
    if (bOK)
      printf("Loaded: Info file \"%s\"\n",
           defnfo[i].c_str());
    }
for (i = 0;
     i < (int)saINames.size();
     i++)
  {
  bool bOK = LoadInfo(saINames[i]);
  printf("%soaded: Info file \"%s\"\n",
         bOK ? "L" : "NOT l",
         saINames[i].c_str());
  }

// parse labels in 2 passes
if (pDasm->GetMemoryArrayCount(false)) Parse(0, false);
if (pDasm->GetMemoryArrayCount(false)) Parse(0, true);
if (pDasm->GetMemoryArrayCount(true))  Parse(1, false);
if (pDasm->GetMemoryArrayCount(true))  Parse(1, true);
// resolve all XXXXXXXX+/-nnnn labels
ResolveRelativeLabels(false);
ResolveRelativeLabels(true);

// set up often used texts and flags
std::string labelDelim = pDasm->GetOption("ldchar");
std::string sComDel = pDasm->GetOption("cchar");
std::string sComBlk = sComDel + " ";
std::string sComHdr(sComDel);
while (sComHdr.size() < 53)
  sComHdr += '*';

// create output file
out = outname.size() ? fopen(outname.c_str(), "w") : stdout;
if (out == NULL)
  out = stdout;
if (out != stdout)
  PrintLine(sComDel +
            sformat(" %s: Disassembler Framework V" DASMFW_VERSION,
                    sDasmName.c_str()));
for (i = 0;                             /* print loaded files                */
     i < (int)saFNames.size();
     i++)
  if (saFNames[i].substr(0, 1) != "-")
    PrintLine(sComDel + " " + saFNames[i]);

// TEST TEST TEST TEST TEST TEST TEST
// #ifdef _DEBUG
#if 0
PrintLine();
PrintLine(sComBlk + "After Parsing:");
PrintLine(sComBlk +
          sformat("Loaded memory areas: begin=%s, end=%s",
                  pDasm->GetOption("begin").c_str(),
                  pDasm->GetOption("end").c_str()));
DumpMem(false);
DumpMem(true);
#endif


// disassembler-specific initialization
DisassembleChanges(NO_ADDRESS, NO_ADDRESS, 0, false, false);

AddrTextArray::iterator it;

for (int bus = 0; bus < 2; bus++)
  {
  bool bDataBus = !!bus;
  if (!pDasm->GetMemoryArrayCount(bDataBus))
    continue;

  // output of labels without data
  DisassembleLabels(sComDel, sComHdr, bDataBus);

  addr_t prevaddr = NO_ADDRESS, prevsz = 0;
  addr_t addr = pDasm->GetMemoryArray(0, bDataBus).GetStart();
  if (!pDasm->IsCellUsed(addr, bDataBus))
    addr = pDasm->GetNextAddr(addr, bDataBus);
  bool bBusHdrOut = false;
  while (addr != NO_ADDRESS)
    {
    if (!bBusHdrOut)
      {
      PrintLine();
      PrintLine(sComHdr);
      PrintLine(sComBlk +
                sformat("Program's %s Areas",
                        (bDataBus) ? "Data Bus Data" : "Code / Data"));
      PrintLine(sComHdr);
      PrintLine();

      bBusHdrOut = true;
      }

    DisassembleChanges(addr, prevaddr, prevsz, false, bDataBus);

    addr_t sz = DisassembleLine(addr, sComDel, sComHdr, labelDelim, bDataBus);

    DisassembleChanges(addr, prevaddr, prevsz, true, bDataBus);

    prevaddr = addr;
    prevsz = sz;
    addr = pDasm->GetNextAddr(addr + sz - 1, bDataBus);
    }

  // disassembler-specific end of area handling
  DisassembleChanges(addr, prevaddr, prevsz, false, bDataBus);
  }

// disassembler-specific closing
DisassembleChanges(NO_ADDRESS, NO_ADDRESS, 0, true, true);

if (out != stdout)
  fclose(out);
if (pDasm) delete pDasm;
return 0;
}

/*****************************************************************************/
/* Parse : go through the loaded memory areas and parse all labels           */
/*****************************************************************************/

bool Application::Parse(int nPass, bool bDataBus)
{
if (!nPass &&
    !pDasm->InitParse(bDataBus))
  return false;

if (!pDasm->GetMemoryArrayCount(bDataBus))
  return true;

addr_t prevaddr = NO_ADDRESS;
addr_t addr = pDasm->GetMemoryArray(0, bDataBus).GetStart();
if (!pDasm->IsCellUsed(addr, bDataBus))
  addr = pDasm->GetNextAddr(addr, bDataBus);

while (addr != NO_ADDRESS)
  {
  addr_t sz = pDasm->Parse(addr, bDataBus);
  prevaddr = addr;
  addr = pDasm->GetNextAddr(addr + sz - 1, bDataBus);
  }

return true;
}

/*****************************************************************************/
/* ResolveRelativeLabels : resolve all XXXXXXXX+/-nnnn labels                */
/*****************************************************************************/

bool Application::ResolveRelativeLabels(bool bDataBus)
{
for (int i = pDasm->GetLabelCount(bDataBus) - 1; i >= 0; i--)
  {
  Label *pLbl = pDasm->LabelAt(i, bDataBus);
  if (!pLbl->IsUsed())
    continue;
  std::string s = pLbl->GetText();
  std::string::size_type p = s.find_first_of("+-");
  addr_t offs;
  if (p != s.npos &&
      pDasm->String2Number(s.substr(p + 1), offs))
    {
    if (s[p] == '+') offs = (addr_t)(-(int32_t)offs);
    pLbl->SetUsed(false);
    pDasm->AddLabel(pLbl->GetAddress() + offs, pLbl->GetType(),
                    s.substr(0, p), true, bDataBus);
    // this might have caused an insertion, so restart here
    i++;
    }
  }

return true;
}

/*****************************************************************************/
/* DisassembleChanges : parses pre-/post-changes for a disassembly line      */
/*****************************************************************************/

bool Application::DisassembleChanges
    (
    addr_t addr,
    addr_t prevaddr,
    addr_t prevsz,
    bool bAfterLine,
    bool bDataBus
    )
{
std::vector<Disassembler::LineChange> changes;
bool bRC = pDasm->DisassembleChanges(addr,
                                     prevaddr, prevsz,
                                     bAfterLine,
                                     changes,
                                     bDataBus);

for (std::vector<Disassembler::LineChange>::size_type i = 0;
     i < changes.size();
     i++)
  {
  PrintLine("", changes[i].smnemo.c_str(), changes[i].sparm.c_str());
  }
return bRC;
}

/*****************************************************************************/
/* DisassembleLabels : disassemble all labels without memory area            */
/*****************************************************************************/

bool Application::DisassembleLabels
    (
    std::string sComDel,
    std::string sComHdr,
    bool bDataBus
    )
{
std::string sComBlk(sComDel + " ");
AddrTextArray::iterator it;
Comment *pComment;
bool bULHOut = false;
for (int l = 0; l < pDasm->GetLabelCount(bDataBus); l++)
  {
  Label *pLbl = pDasm->LabelAt(l, bDataBus);
  addr_t laddr = pLbl->GetAddress();
  if (pLbl->IsUsed() &&
      pDasm->GetMemType(laddr, bDataBus) == Untyped)
    {
    std::string smnemo, sparm;
    if (pDasm->DisassembleLabel(laddr, smnemo, sparm, bDataBus))
      {
      // header, if not yet done
      if (!bULHOut)
        {
        PrintLine();
        PrintLine(sComHdr);
        PrintLine(sComBlk + "Used Labels");
        PrintLine(sComHdr);
        PrintLine();
        bULHOut = true;
        }
      // comments before line
      pComment = GetFirstComment(laddr, it, false, bDataBus);
      while (pComment)
        {
        PrintLine(pComment->GetText());
        pComment = GetNextComment(laddr, it, false, bDataBus);
        }

      std::string slabel = pDasm->Label2String(laddr, true, laddr, bDataBus);

      // the line itself
      pComment = GetFirstLComment(laddr, it, bDataBus);
      std::string scomment = pComment ? pComment->GetText() : "";
      if (scomment.size()) scomment = sComBlk + scomment;
      PrintLine(slabel, smnemo, sparm, scomment);
      if (pComment)
        {
        while ((pComment = GetNextLComment(laddr, it, bDataBus)) != NULL)
          PrintLine("", "", "", sComBlk + pComment->GetText());
        }

      // comments after line
      pComment = GetFirstComment(laddr, it, true, bDataBus);
      while (pComment)
        {
        PrintLine(pComment->GetText());
        pComment = GetNextComment(laddr, it, true, bDataBus);
        }
      }
    }
  }

return true;
}

/*****************************************************************************/
/* DisassembleLine : create a line of disassembler output (+comments)        */
/*****************************************************************************/

addr_t Application::DisassembleLine
    (
    addr_t addr,
    std::string sComDel,
    std::string sComHdr,
    std::string labelDelim,
    bool bDataBus
    )
{
std::string sLabel, sMnemo, sParms, sComBlk(sComDel + " ");
AddrTextArray::iterator it;
Comment *pComment;
bool bWithComments = showHex || showAsc || showAddr;

// comments before line
pComment = GetFirstComment(addr, it, false, bDataBus);
while (pComment)
  {
  PrintLine(pComment->GetText());
  pComment = GetNextComment(addr, it, false, bDataBus);
  }

// the line itself
Label *p = pDasm->FindLabel(addr, Untyped, bDataBus);
if (p && p->IsUsed())
  sLabel = pDasm->Label2String(addr, true, addr, bDataBus) +
           labelDelim;
pComment = GetFirstLComment(addr, it, bDataBus);
int maxparmlen = (bWithComments || pComment) ? 24 : 52;
addr_t sz = pDasm->Disassemble(addr, sMnemo, sParms, maxparmlen, bDataBus);

std::string scomment;
if (showAddr)
  scomment += sformat("%0*X%s",
                      pDasm->GetCodeBits() / 4, addr,
                      (showHex || showAsc) ? ": " : " ");
if ((showHex || showAsc) && !pDasm->IsBss(addr, bDataBus))
  {
  std::string sHex, sAsc("\'");
  addr_t i;
  for (i = 0; i < sz; i++)
    {
    uint8_t c = pDasm->GetUByte(addr + i, bDataBus);
    sHex += sformat("%02X ", c);
    sAsc += (isprint(c)) ? c : '.'; 
    }
  sAsc += '\'';
  if (showHex || showAsc)
    {
    for (; i < 5; i++)
      {
      if (showHex && showAsc)
        sHex += "   ";
      if (showAsc && pComment)
        sAsc += ' ';
      }
    }

  if (showHex)
    scomment += sHex;
  if (showAsc)
    scomment += sAsc;
  }
scomment += pComment ? pComment->GetText() : "";
if (scomment.size()) scomment = sComBlk + scomment;
PrintLine(sLabel, sMnemo, sParms, scomment);
if (pComment)
  {
// following line comments
  while ((pComment = GetNextLComment(addr, it, bDataBus)) != NULL)
    PrintLine("", "", "", sComBlk + pComment->GetText());
  }

// comments after line
pComment = GetFirstComment(addr, it, true, bDataBus);
while (pComment)
  {
  PrintLine(pComment->GetText());
  pComment = GetNextComment(addr, it, true, bDataBus);
  }

return sz;
}

/*****************************************************************************/
/* PrintLine : prints a formatted line to the output                         */
/*****************************************************************************/

bool Application::PrintLine
    (
    std::string sLabel,
    std::string smnemo,
    std::string sparm,
    std::string scomment
    )
{
// for now: VERY primitive output
int nLen = 0;
if (sLabel.size())
  nLen += fprintf(out, "%s ", sLabel.c_str());
if (smnemo.size())
  {
  if (nLen < 8) nLen += fprintf(out, "%*s", 8 - nLen, "");
  nLen += fprintf(out, "%s ", smnemo.c_str());
  }
if (sparm.size())
  {
  if (nLen < 16) nLen += fprintf(out, "%*s", 16 - nLen, "");
  nLen += fprintf(out, "%s ", sparm.c_str());
  }
if (scomment.size())
  {
  if (nLen < 41) nLen += fprintf(out, "%*s", 41 - nLen, "");
  nLen += fprintf(out, "%s ", scomment.c_str());
  }
nLen += fprintf(out, "\n");
return nLen > 0;
}

/*****************************************************************************/
/* InfoHelp : pass back help for info files                                  */
/*****************************************************************************/

bool Application::InfoHelp()
{
return true;
}

/*****************************************************************************/
/* ParseInfoRange : parses a range definition from an info file              */
/*****************************************************************************/

int Application::ParseInfoRange(std::string value, addr_t &from, addr_t &to)
{
int n = pDasm->String2Range(value, from, to);
if (n < 1)
  from = NO_ADDRESS;
if (from != NO_ADDRESS)
  {
  addr_t *pmap = remaps[bInfoBus].getat(from);
  if (pmap) from += *pmap;
  if ((bInfoBus &&
       (from < pDasm->GetLowestDataAddr() ||
        from > pDasm->GetHighestDataAddr())) ||
      (!bInfoBus &&
       (from < pDasm->GetLowestCodeAddr() ||
        from > pDasm->GetHighestCodeAddr())))
    {
    from = NO_ADDRESS;
    n = 0;
    }
  }
if (n < 2)
  to = from;
else if (to != NO_ADDRESS)
  {
  addr_t *pmap = remaps[bInfoBus].getat(to);
  if (pmap) to += *pmap;
  if ((bInfoBus &&
       (to < pDasm->GetLowestDataAddr() ||
        to > pDasm->GetHighestDataAddr())) ||
      (!bInfoBus &&
       (to < pDasm->GetLowestCodeAddr() ||
        to > pDasm->GetHighestCodeAddr())))
    {
    to = NO_ADDRESS;
    n = 0;
    }
  }
return n;
}

/*****************************************************************************/
/* triminfo : trims an info line's value, cutting at comment character       */
/*****************************************************************************/

static std::string triminfo
    (
    std::string s,
    bool bCutComment = true,
    bool bUnescape = true,
    bool bDotStart = false
    )
{
// copied from trim()
if (s.empty()) return s;
std::string::size_type from = s.find_first_not_of(" ");
if (from == s.npos)
  return "";
// up to here. The rest is info-specific
if (bDotStart && s[from] == '.')        /* '.' can allow leading blanks      */
  from++;                               /* (redundant f9dasm behavior)       */
std::string sout;
while (from < s.size())                 /* copy the rest, with unescaping    */
  {
  if (bUnescape && s[from] == '\\' && from + 1 < s.size())
    sout += s[++from];
  else if (bCutComment && s[from] == '*')
    break;
  else
    sout += s[from++];
  }

if (sout.empty()) return sout;
return sout.substr(0, sout.find_last_not_of(" ") + 1);
}

/*****************************************************************************/
/* LoadInfo : loads an information file                                      */
/*****************************************************************************/

bool Application::LoadInfo
    (
    std::string fileName,
    std::vector<std::string> &loadStack,
    bool bProcInfo
    )
{
if (fileName == "help")                 /* if help is needed                 */
  return InfoHelp();                    /* give it.                          */

if (!pDasm && bProcInfo)                /* no disassembler, no work.         */
  return false;
                                        /* inhibit recursion                 */
for (std::vector<std::string>::const_iterator lsi = loadStack.begin();
     lsi != loadStack.end();
     lsi++)
  if (*lsi == fileName)
    return false;

enum InfoCmd
  {
  infoUnknown = -1,                     /* unknown info command              */
  // bus selection
  infoBus,                              /* BUS {code|data}                   */
  // memory types
  infoCode,                             /* CODE addr[-addr]                  */
  infoData,                             /* DATA addr[-addr]                  */
  infoConstant,                         /* CONST addr[-addr]                 */
  infoCVector,                          /* CVEC[TOR] addr[-addr]             */
  infoDVector,                          /* DVEC[TOR] addr[-addr]             */
  infoRMB,                              /* RMB addr[-addr]                   */
  infoUnused,                           /* UNUSED addr[-addr]                */
  // cell sizes
  infoByte,                             /* BYTE addr[-addr]                  */
  infoWord,                             /* WORD addr[-addr]                  */
  infoDWord,                            /* DWORD addr[-addr]                 */
  // cell display types
  infoBinary,                           /* BINARY addr[-addr]                */
  infoChar,                             /* CHAR addr[-addr]                  */
  infoOct,                              /* OCT addr[-addr]                   */
  infoDec,                              /* DEC addr[-addr]                   */
  infoHex,                              /* HEX addr[-addr]                   */
  infoFloat,                            /* FLOAT addr[-addr]                 */
  infoDouble,                           /* DOUBLE addr[-addr]                */
  infoTenBytes,                         /* TENBYTES addr[-addr]              */
  // break before cell
  infoBreak,                            /* BREAK addr[-addr]                 */
  infoUnBreak,                          /* UNBREAK addr[-addr]               */
  // relative address
  infoRelative,                         /* RELATIVE addr[-addr] rel          */
  infoUnRelative,                       /* UNRELATIVE addr[-addr]            */
  // label handling
  infoLabel,                            /* LABEL addr[-addr] label           */
  infoUsedLabel,                        /* USEDLABEL addr[-addr] [label]     */
  infoUnlabel,                          /* UNLABEL addr[-addr]               */
  // phasing support
  infoPhase,                            /* PHASE addr[-addr] [+|-]phase      */
  infoUnphase,                          /* UNPHASE addr[-addr]               */

  // handled outside disassembler engine:
  infoInclude,                          /* INCLUDE infofilename              */
  infoOption,                           /* OPTION name data                  */
  infoFile,                             /* FILE filename                     */
  infoRemap,                            /* REMAP addr[-addr] offs            */
  // comment handling
  infoComment,                          /* COMMENT addr[-addr] comment       */
  infoPrepComm,                         /* PREPCOMM [addr[-addr]] comment    */
  infoInsert,                           /* INSERT addr[-addr] text           */
  infoPrepend,                          /* PREPEND [addr[-addr]] line        */
  infoLComment,                         /* LCOMMENT addr[-addr] [.]lcomment  */
  infoPrepLComm,                        /* PREPLCOMM addr[-addr] [.]lcomment */
  infoUncomment,                        /* UNCOMMENT addr[-addr]             */
  infoUnLComment,                       /* UNLCOMMENT addr[-addr]            */
  // patching support
  infoPatch,                            /* PATCH addr [byte]*                */
  infoPatchWord,                        /* PATCHW addr [word]*               */
  infoPatchDWord,                       /* PATCHDW addr [dword]*             */
  infoPatchFloat,                       /* PATCHF addr [float]*              */

  // end info file processing
  infoEnd,                              /* END (processing this file)        */
  };
static struct                           /* structure to convert key to type  */
  {
  const char *name;
  InfoCmd cmdType;
  } sKey[] =
  {
  // bus selection
  { "BUS",          infoBus },
  // memory types
  { "CODE",         infoCode },
  { "DATA",         infoData },
  { "CONSTANT",     infoConstant },
  { "CONST",        infoConstant },
  { "CVECTOR",      infoCVector },
  { "CVEC",         infoCVector },
  { "DVECTOR",      infoDVector },
  { "DVEC",         infoDVector },
  { "RMB",          infoRMB },
  { "BSS",          infoRMB },
  { "UNUSED",       infoUnused },
  // cell sizes
  { "BYTE",         infoByte },
  { "WORD",         infoWord },
  { "DWORD",        infoDWord },
  // cell display types
  { "BIN",          infoBinary },
  { "BINARY",       infoBinary },
  { "CHAR",         infoChar },
  { "CHARACTER",    infoChar },
  { "OCTAL",        infoOct },
  { "OCT",          infoOct },
  { "DECIMAL",      infoDec },
  { "DEC",          infoDec },
  { "HEXADECIMAL",  infoHex },
  { "SEDECIMAL",    infoHex },
  { "HEX",          infoHex },
  { "FLOAT",        infoFloat },
  { "DOUBLE",       infoDouble },
  { "TENBYTES",     infoTenBytes },
  // break before cell
  { "BREAK",        infoBreak },
  { "UNBREAK",      infoUnBreak },
  // relative address
  { "RELATIVE",     infoRelative },
  { "REL",          infoRelative },
  { "UNRELATIVE",   infoUnRelative },
  { "UNREL",        infoUnRelative },
  // label handling
  { "LABEL",        infoLabel },
  { "USED",         infoUsedLabel },
  { "USEDLABEL",    infoUsedLabel },
  { "UNLABEL",      infoUnlabel },
  // phasing support
  { "PHASE",        infoPhase },
  { "UNPHASE",      infoUnphase },

  // handled outside disassembler engine:
  { "INCLUDE",      infoInclude },
  { "OPTION",       infoOption },
  { "FILE",         infoFile },
  { "REMAP",        infoRemap },
  // comment handling
  { "COMMENT",      infoComment },
  { "COMM",         infoComment },
  { "PREPCOMMENT",  infoPrepComm },
  { "PREPCOMM",     infoPrepComm },
  { "INSERT",       infoInsert },
  { "PREPEND",      infoPrepend },
  { "LCOMMENT",     infoLComment },
  { "LCOMM",        infoLComment },
  { "PREPLCOMMENT", infoPrepLComm },
  { "PREPLCOMM",    infoPrepLComm },
  { "UNCOMMENT",    infoUncomment },
  { "UNCOMM",       infoUncomment },
  { "UNLCOMMENT",   infoUnLComment },
  { "UNLCOMM",      infoUnLComment },
  // patching support
  { "PATCH",        infoPatch },
  { "PATCHW",       infoPatchWord },
  { "PATCHDW",      infoPatchDWord },
  { "PATCHF",       infoPatchFloat },

  // end info file processing
  { "END",          infoEnd },
  };

                                        /* that's definitely a text file     */
FILE *fp = fopen(fileName.c_str(), "r");
if (!fp)
  return false;

int fc;
std::string line;
bool bMod = false, bEnd = false;
do
  {
  fc = fgetc(fp);
  // skip leading whitespace
  if ((fc == ' ' || fc == '\t') && !line.size())
    continue;
  if (fc == '+' && !line.size())
    {
    bMod = true;
    continue;
    }
  if (fc != '\n' && fc != EOF)
    {
    line += (char)fc;
    continue;
    }
  // newline or end of file encountered
  if (line.size() && line[0] != '*')    /* if line has contents              */
    {
    std::string key, value;
    std::string::size_type idx = line.find_first_of(" \t");
    if (idx == line.npos) idx = line.size();
    key = uppercase(line.substr(0, idx));
    value = trim(line.substr(idx));

    InfoCmd cmdType = infoUnknown;
    for (int i = 0; i < _countof(sKey); i++)
      if (key == sKey[i].name)
        {
        cmdType = sKey[i].cmdType;
        break;
        }

    bool bTgtBus = bInfoBus;            /* target bus identification         */
    idx = value.find_first_of(" \t");
    if (idx != value.npos &&
        lowercase(value.substr(0, idx)) == "bus")
      {
      std::string bval(trim(value.substr(idx)));
      idx = bval.find_first_of(" \t");
      if (idx != bval.npos)
        {
        std::string tgtbus = lowercase(bval.substr(0, idx));
        bval = trim(bval.substr(idx));
        if (tgtbus == "code" || tgtbus == "0")
          {
          bTgtBus = false;
          value = bval;
          }
        else if (tgtbus == "data" || tgtbus == "1")
          {
          bTgtBus = true;
          value = bval;
          }
        }
      }

    if (pDasm)                          /* let disassembler have a go at it  */
      {
      addr_t from, to;                  /* address range has to be first!    */
      ParseInfoRange(value, from, to);
      if (pDasm->ProcessInfo(key, value, from, to, bProcInfo, bInfoBus))
        cmdType = infoUnknown;
      }

    if (!bProcInfo)                     /* if just loading includes and files*/
      {
      if (cmdType != infoInclude &&
          cmdType != infoOption &&
          cmdType != infoFile)
        cmdType = infoUnknown;          /* ignore all unwanted command types */
      }
    else                                /* if processing complete file       */
      {
      if (cmdType == infoFile)
        cmdType = infoUnknown;          /* ignore all unwanted command types */
      }

    switch (cmdType)
      {
      case infoBus :                    /* BUS {code|data}                   */
        {
        std::string s = lowercase(value);
        if (s == "code" || s == "0")
          bInfoBus = false;
        else if (s == "data" || s == "1")
          bInfoBus = true;
        }
        break;
      case infoInclude :                /* INCLUDE filename                  */
        {
        char delim1 = ' ', delim2 = '\t';
        std::string fn;
        std::string::size_type i = 0;
        if (value[i] == '\"' || value[i] == '\'')
          {
          delim1 = value[i++];
          delim2 = '\0';
          }
        for (; i < value.size() && value[i] != delim1 && value[i] != delim2; i++)
          fn += value[i];
        loadStack.push_back(fileName);
        LoadInfo(fn, loadStack, bProcInfo);
        loadStack.pop_back();
        }
        break;
      case infoFile :                   /* FILE filename [offset]            */
        {
        char delim1 = ' ', delim2 = '\t';
        std::string fn;
        std::string::size_type i = 0;
        if (value[i] == '\"' || value[i] == '\'')
          {
          delim1 = value[i++];
          delim2 = '\0';
          }
        for (; i < value.size() && value[i] != delim1 && value[i] != delim2; i++)
          fn += value[i];
        if (i < value.size())
          value = trim(value.substr(i + 1));
        else
          value.clear();
        addr_t offs, ign;
        ParseInfoRange(value, offs, ign);
        if (offs != NO_ADDRESS)
          saFNames.push_back(sformat("-offset:0x%x", offs));
        saFNames.push_back(fn);
        }
        break;
      case infoOption :                 /* OPTION name value                 */
        {
        std::string option;
        idx = value.find_first_of(" \t");
        if (idx == value.npos) idx = value.size();
        option = value.substr(0, idx);
        value = trim(value.substr(idx));
        ParseOption(option, value, !bProcInfo);
        }
        break;
      case infoRemap :                  /* REMAP addr[-addr] offset          */
        {
        std::string range;
        idx = value.find_first_of(" \t");
        if (idx == value.npos) idx = value.size();
        range = value.substr(0, idx);
        value = trim(value.substr(idx));
        addr_t from, to, off;
        // allow remapped remaps :-)
        if (ParseInfoRange(range, from, to) >= 1 &&
            pDasm->String2Number(value, off))
          {
          remaps[bInfoBus].AddMemory(from, to + 1 - from);
          for (addr_t scanned = from;
               scanned >= from && scanned <= to;
               scanned++)
            *remaps[bInfoBus].getat(scanned) += off;
          }
        }
        break;
      // memory types
      case infoCode :                   /* CODE addr[-addr]                  */
      case infoData :                   /* DATA addr[-addr]                  */
      case infoConstant :               /* CONST addr[-addr]                 */
      case infoCVector :                /* CVEC[TOR] addr[-addr]             */
      case infoDVector :                /* DVEC[TOR] addr[-addr]             */
      case infoRMB :                    /* RMB addr[-addr]                   */
      case infoUnused :                 /* UNUSED addr[-addr]                */
      // cell sizes
      case infoByte :                   /* BYTE addr[-addr]                  */
      case infoWord :                   /* WORD addr[-addr]                  */
      case infoDWord :                  /* DWORD addr[-addr]                 */
      // cell display types
      case infoBinary :                 /* BINARY addr[-addr]                */
      case infoChar :                   /* CHAR addr[-addr]                  */
      case infoOct :                    /* OCT addr[-addr]                   */
      case infoDec :                    /* DEC addr[-addr]                   */
      case infoHex :                    /* HEX addr[-addr]                   */
      // combined size + type
      case infoFloat :                  /* FLOAT addr[-addr]                 */
      case infoDouble :                 /* DOUBLE addr[-addr]                */
      case infoTenBytes :               /* TENBYTES addr[-addr]              */
      // break before
      case infoBreak :                  /* BREAK addr[-addr]                 */
      case infoUnBreak:                 /* UNBREAK addr[-addr]               */
        {
        addr_t from, to, tgtaddr;
        if (ParseInfoRange(value, from, to) >= 1)
          {
          for (addr_t scanned = from;
               scanned >= from && scanned <= to;
               scanned++)
            {
            MemoryType ty = pDasm->GetMemType(scanned, bInfoBus);
            int sz;
            if (ty != Untyped)
              {
              switch (cmdType)
                {
                case infoCode :
                  pDasm->SetMemType(scanned, Code, bInfoBus);
                  break;
                case infoData :
                  pDasm->SetMemType(scanned, Data, bInfoBus);
                  break;
                case infoConstant :
                  pDasm->SetMemType(scanned, Const, bInfoBus);
                  break;
                case infoRMB :
                  pDasm->SetMemType(scanned, Bss, bInfoBus);
                  break;
                case infoUnused :
                  pDasm->SetMemType(scanned, Untyped, bInfoBus);
                  pDasm->SetCellUsed(scanned, false, bInfoBus);
                case infoByte :
                  pDasm->SetCellSize(scanned, 1, bInfoBus);
                  break;
                case infoWord :
                  pDasm->SetCellSize(scanned, 2, bInfoBus);
                  break;
                case infoDWord :
                  pDasm->SetCellSize(scanned, 4, bInfoBus);
                  break;
                case infoBinary :
                  pDasm->SetDisplay(scanned, MemAttribute::Binary, bInfoBus);
                  break;
                case infoChar :
                  pDasm->SetDisplay(scanned, MemAttribute::Char, bInfoBus);
                  break;
                case infoOct :
                  pDasm->SetDisplay(scanned, MemAttribute::Octal, bInfoBus);
                  break;
                case infoDec :
                  pDasm->SetDisplay(scanned, MemAttribute::Decimal, bInfoBus);
                  break;
                case infoHex :
                  pDasm->SetDisplay(scanned, MemAttribute::Hex, bInfoBus);
                  break;
                case infoFloat :
                  pDasm->SetCellSize(scanned, 4, bInfoBus);
                  pDasm->SetCellType(scanned, MemAttribute::Float, bInfoBus);
                  break;
                case infoDouble :
                  pDasm->SetCellSize(scanned, 8, bInfoBus);
                  pDasm->SetCellType(scanned, MemAttribute::Float, bInfoBus);
                  break;
                case infoTenBytes :
                  pDasm->SetCellSize(scanned, 10, bInfoBus);
                  pDasm->SetCellType(scanned, MemAttribute::Float, bInfoBus);
                  scanned += 9;
                  break;
                case infoBreak :
                  pDasm->SetBreakBefore(scanned, true, bInfoBus);
                  break;
                case infoUnBreak:
                  pDasm->SetBreakBefore(scanned, false, bInfoBus);
                  break;
                case infoCVector :
                  // a code vector defines a table of code pointers
                  pDasm->SetMemType(scanned, Data, bInfoBus);
                  sz = pDasm->GetCodePtrSize();
                  pDasm->SetCellSize(scanned, sz, bInfoBus);
                  if (sz == 1)
                    tgtaddr = pDasm->GetUByte(scanned, bInfoBus);
                  else if (sz == 2)
                    tgtaddr = pDasm->GetUWord(scanned, bInfoBus);
                  else if (sz == 4)
                    tgtaddr = pDasm->GetUDWord(scanned, bInfoBus);
                  pDasm->AddLabel(tgtaddr, Code,
                                  sformat("Z%0*Xvia%0*X",
                                          sz*2, tgtaddr,
                                          sz*2, scanned),
                                  true, bTgtBus);
                  scanned += sz - 1;
                  break;
                case infoDVector :
                  // a data vector defines a table of data pointers
                  pDasm->SetMemType(scanned, Data, bInfoBus);
                  sz = pDasm->GetDataPtrSize();
                  pDasm->SetCellSize(scanned, sz, bInfoBus);
                  if (sz == 1)
                    tgtaddr = pDasm->GetUByte(scanned, bInfoBus);
                  else if (sz == 2)
                    tgtaddr = pDasm->GetUWord(scanned, bInfoBus);
                  else if (sz == 4)
                    tgtaddr = pDasm->GetUDWord(scanned, bInfoBus);
                  pDasm->AddLabel(tgtaddr, Code,
                                  sformat("M%0*Xvia%0*X",
                                          sz*2, tgtaddr,
                                          sz*2, scanned),
                                  true, bTgtBus);
                  scanned += sz - 1;
                  break;
                }
              }
            }
          }
        }
        break;
      case infoRelative :               /* RELATIVE addr[-addr] rel          */
        {
        std::string range;
        idx = value.find_first_of(" \t");
        if (idx == value.npos) idx = value.size();
        range = value.substr(0, idx);
        value = trim(value.substr(idx));
        addr_t from, to, rel;
        if (ParseInfoRange(range, from, to) >= 1 &&
            pDasm->String2Number(value, rel))
          {
          pDasm->AddRelative(from, to - from + 1, NULL, bInfoBus);
          for (addr_t scanned = from;
               scanned >= from && scanned <= to;
               scanned++)
           pDasm->SetRelative(scanned, rel, bInfoBus);
          }
        }
        break;
      case infoUnRelative :             /* UNRELATIVE addr[-addr]            */
        {
        addr_t from, to;
        if (ParseInfoRange(value, from, to) >= 1)
          {
          for (addr_t scanned = from;
               scanned >= from && scanned <= to;
               scanned++)
           pDasm->SetRelative(scanned, 0, bInfoBus);
          }
        }
        break;
      case infoLabel :                  /* LABEL addr[-addr] label           */
      case infoUsedLabel :              /* USEDLABEL addr[-addr] [label]     */
        {
        addr_t from, to;
        std::string range;
        idx = value.find_first_of(" \t");
        if (idx == value.npos) idx = value.size();
        range = value.substr(0, idx);
        value = (idx < value.size()) ? triminfo(value.substr(idx), true, false) : "";
        idx = value.find_first_of(" \t");
        if (idx != value.npos)
          value = trim(value.substr(0, idx));
        bool bTextOk = (cmdType == infoUsedLabel) || value.size();
        if (ParseInfoRange(range, from, to) >= 1 && bTextOk)
          {
          for (addr_t scanned = from;
               scanned >= from && scanned <= to;
               scanned++)
            {
            pDasm->AddLabel(
                scanned, Untyped, 
                (scanned == from ||
                 (cmdType == infoUsedLabel && value.empty())) ?
                     value :
                     sformat("%s+%d", value.c_str(), scanned - from),
                cmdType == infoUsedLabel,
                bInfoBus);
            }
          }
        }
        break;
      case infoUnlabel :                /* UNLABEL addr[-addr]               */
        {
        addr_t from, to;
        if (ParseInfoRange(value, from, to) >= 1)
          {
          for (int i = pDasm->GetLabelCount(bInfoBus) - 1; i >= 0; i--)
            {
            addr_t laddr = pDasm->LabelAt(i, bInfoBus)->GetAddress();
            if (laddr < from) break;
            if (laddr >= from && laddr <= to)
              pDasm->RemoveLabelAt(i, bInfoBus);
            }
          }
        }
        break;
      case infoPhase :                  /* PHASE addr[-addr] [+|-]phase      */
        {
        std::string range;
        idx = value.find_first_of(" \t");
        if (idx == value.npos) idx = value.size();
        range = value.substr(0, idx);
        value = trim(value.substr(idx));
        char sign = value.size() ? value[0] : 0;
        bool bSigned = (sign == '+' || sign == '-');
        addr_t from, to, phase;
        if (ParseInfoRange(range, from, to) >= 1 &&
            pDasm->String2Number(value, phase))
          {
          // This might cause garbage if overlapping or disjunct areas of
          // different phase are specified. Well, so be it. Don't do it.
          // The alternative would be to reallocate the phase area for each
          // and every byte.
          TMemory<addr_t, addr_t> *pFrom = pDasm->FindPhase(from, bInfoBus);
          TMemory<addr_t, addr_t> *pTo = pDasm->FindPhase(to, bInfoBus);
          if (!pFrom || !pTo)
            {
            // presumably, it's the same phase as others, so make sure
            // to create a unique area
            pDasm->AddPhase(from, to - from + 1, NO_ADDRESS, bInfoBus);
            pFrom = pDasm->FindPhase(from, bInfoBus);
            pFrom->SetType(phase);
            }
          for (addr_t scanned = from;
               scanned >= from && scanned <= to;
               scanned++)
            {
            TMemory<addr_t, addr_t> *pArea = pDasm->FindPhase(scanned, bInfoBus);
            if (pArea)
              {
              if (pArea->GetType() == phase)
                pDasm->SetPhase(scanned, NO_ADDRESS, bInfoBus);
              else
                pDasm->SetPhase(scanned,
                                bSigned ? pArea->GetType() - phase : phase,
                                bInfoBus);
              }
            }
          }
        }
        break;
      case infoUnphase :                /* UNPHASE addr[-addr]               */
        {
        addr_t from, to;
        if (ParseInfoRange(value, from, to) >= 1)
          {
          for (addr_t scanned = from;
               scanned >= from && scanned <= to;
               scanned++)
            pDasm->SetPhase(scanned, DEFAULT_ADDRESS, bInfoBus);
          }
        }
        break;
      case infoComment :                /* COMMENT addr[-addr] comment       */
      case infoPrepComm :               /* PREPCOMM [addr[-addr]] comment    */
      case infoInsert :                 /* INSERT addr[-addr] text           */
      case infoPrepend :                /* PREPEND [addr[-addr]] line        */
      case infoLComment :               /* LCOMMENT addr[-addr] [.]lcomment  */
      case infoPrepLComm :              /* PREPLCOMM addr[-addr] [.]lcomment */
        {
        // special: "AFTER" as 1st string (line comment ignores it)
        bool bAfter = false;
        bool bPrepend = cmdType == infoPrepComm ||
                         cmdType == infoPrepend ||
                         cmdType == infoPrepLComm;
        bool bIsComm = cmdType == infoComment ||
                       cmdType == infoPrepComm;
        idx = value.find_first_of(" \t");
        if (idx == value.npos) idx = value.size();
        std::string after = lowercase(value.substr(0, idx));
        if (after == "after")
          {
          bAfter = true;
          value = (idx < value.size()) ? trim(value.substr(idx)) : "";
          }
        addr_t from, to;
        std::string range;
        idx = value.find_first_of(" \t");
        if (idx == value.npos) idx = value.size();
        range = value.substr(0, idx);
        value = (idx >= value.size()) ? "" :
          triminfo(value.substr(idx), true, true, true);
        if (bIsComm && value.size())
          value = pDasm->GetOption("cchar") + " " + value;
        if (ParseInfoRange(range, from, to) >= 1)
          {
          for (addr_t scanned = from;
               scanned >= from && scanned <= to;
               scanned++)
            {
                                        /* make sure comment breaks output   */
            pDasm->SetBreakBefore(scanned, true, bInfoBus);
            switch (cmdType)
              {
              case infoComment :        /* COMMENT addr[-addr] comment       */
              case infoPrepComm :       /* PREPCOMM [addr[-addr]] comment    */
              case infoInsert :         /* INSERT addr[-addr] text           */
              case infoPrepend :        /* PREPEND [addr[-addr]] line        */
                AddComment(scanned, bAfter, value, bPrepend, bInfoBus);
                break;
              case infoLComment :       /* LCOMMENT addr[-addr] [.]lcomment  */
              case infoPrepLComm :      /* PREPLCOMM addr[-addr] [.]lcomment */
                AddLComment(scanned, value, bPrepend, bInfoBus);
                break;
              }
            }
          }
        }
        break;
      case infoUncomment :              /* UNCOMMENT [BEFORE] addr[-addr]    */
      case infoUnLComment :             /* UNLCOMMENT addr[-addr]            */
        {
        // special: "AFTER" as 1st string (line comment ignores it)
        bool bAfter = false;
        idx = value.find_first_of(" \t");
        if (idx == value.npos) idx = value.size();
        std::string after = lowercase(value.substr(0, idx));
        if (after == "after")
          {
          bAfter = true;
          value = (idx < value.size()) ? trim(value.substr(idx)) : "";
          }
        addr_t from, to;
        if (ParseInfoRange(value, from, to) >= 1)
          {
          int i;
          if (cmdType == infoUncomment)
            {
            for (i = (int)comments[bInfoBus][bAfter].size() - 1; i >= 0; i--)
              {
              AddrText *c = comments[bInfoBus][bAfter].at(i);
              addr_t caddr = c->GetAddress();
              if (caddr < from) break;
              if (caddr >= from && caddr <= to)
                comments[bInfoBus][bAfter].erase(comments[bInfoBus][bAfter].begin() + i);
              }
            }
          else
            {
            for (i = (int)lcomments[bInfoBus].size() - 1; i >= 0; i--)
              {
              AddrText *c = lcomments[bInfoBus].at(i);
              addr_t caddr = c->GetAddress();
              if (caddr < from) break;
              if (caddr >= from && caddr <= to)
                lcomments[bInfoBus].erase(lcomments[bInfoBus].begin() + i);
              }
            }
          }
        }
        break;
      case infoPatch :                  /* PATCH addr [byte]*                */
      case infoPatchWord :              /* PATCHW addr [word]*               */
      case infoPatchDWord :             /* PATCHDW addr [dword]*             */
      case infoPatchFloat :             /* PATCHF addr [float]*              */
        {
        addr_t from, to;
        std::string range;
        idx = value.find_first_of(" \t");
        if (idx == value.npos) idx = value.size();
        range = value.substr(0, idx);
        value = (idx >= value.size()) ? "" :
          triminfo(value.substr(idx), true, false);
        if (ParseInfoRange(range, from, to) >= 1)
          {
          // "to" is not interesting here.
          do
            {
            if (from < pDasm->GetLowestCodeAddr() ||
                from > pDasm->GetHighestCodeAddr())
              break;
            idx = value.find_first_of(" \t");
            if (idx == value.npos) idx = value.size();
            std::string item = value.substr(0, idx);
            value = (idx >= value.size()) ? "" : trim(value.substr(idx));
            to = 0;
            switch (cmdType)
              {
              case infoPatch :          /* PATCH addr [byte]*                */
                if (sscanf(item.c_str(), "%x", &to) == 1)
                  {
                  if (pDasm->GetMemIndex(from, bInfoBus) == NO_ADDRESS)
                    pDasm->AddMemory(from, 1, Code, NULL, bInfoBus);
                  pDasm->SetUByte(from++, (uint8_t)to, bInfoBus);
                  }
                else
                  from = NO_ADDRESS;
                break;
              case infoPatchWord :      /* PATCHW addr [word]*               */
                if (sscanf(item.c_str(), "%x", &to) == 1)
                  {
                  if (pDasm->GetMemIndex(from, bInfoBus) == NO_ADDRESS)
                    pDasm->AddMemory(from, 2, Data, NULL, bInfoBus);
                  pDasm->SetUWord(from, (uint16_t)to, bInfoBus);
                  from += 2;
                  }
                else
                  from = NO_ADDRESS;
                break;
              case infoPatchDWord :     /* PATCHDW addr [dword]*             */
                if (sscanf(item.c_str(), "%x", &to) == 1)
                  {
                  if (pDasm->GetMemIndex(from, bInfoBus) == NO_ADDRESS)
                    pDasm->AddMemory(from, 4, Data, NULL, bInfoBus);
                  pDasm->SetUDWord(from, (uint32_t)to, bInfoBus);
                  from += 4;
                  }
                else
                  from = NO_ADDRESS;
                break;
              case infoPatchFloat :     /* PATCHF addr [float]*              */
                {
                double d;
                if (sscanf(item.c_str(), "%lf", &d) == 1)
                  {
                  int sz;
                  if (pDasm->GetMemIndex(from, bInfoBus) == NO_ADDRESS)
                    {
                    pDasm->AddMemory(from, 4, Data, NULL, bInfoBus);
                    sz = 4;
                    }
                  else
                    sz = pDasm->GetCellSize(from, bInfoBus);
                  if (sz == 4)
                    pDasm->SetFloat(from, (float)d, bInfoBus);
                  else if (sz == 8)
                    pDasm->SetDouble(from, d, bInfoBus);
                  // Tenbytes ... no, sorry. Not yet. Maybe never.
                  else
                    from = NO_ADDRESS;
                  }
                else
                  from = NO_ADDRESS;
                }
                break;
              }

            } while (value.size() && from != NO_ADDRESS);
          }
        }
        break;
      case infoEnd :
        bEnd = true;
        break;
      }
    }
  line.clear();
  bMod = false;
  } while (fc != EOF && !bEnd);

fclose(fp);
return true;
}

/*****************************************************************************/
/* ParseOption : parses an option and associated value                       */
/*****************************************************************************/

int Application::ParseOption
    (
    std::string option,                 /* option name                       */
    std::string value,                  /* new option value                  */
    bool bSetDasm,                      /* flag whether set disassembler     */
    bool bProcInfo                      /* flag whether to fully process info*/
    )
{
std::string lvalue(lowercase(value));
int bnvalue = (lvalue == "off") ? 0 : (lvalue == "on") ? 1 : atoi(value.c_str());

if (bSetDasm)                           /* 1st pass: find only dasm          */
  {
  if (option == "dasm")
    {
    if (pDasm) delete pDasm;            /* use the last one                  */
    pDasm = CreateDisassembler(lowercase(value), &iDasm);
    if (!pDasm)
      printf("Unknown disassembler \"%s\"\n", value.c_str());
    return 1;
    }
  else if (option != "info")            /* recurse through infofiles         */
    return 0;                           /* and ignore anything else          */
  }
else if (option == "dasm")              /* 2nd pass: ignore dasm             */
  return 1;

if (option == "?" || option == "help")
  Help();
else if (option == "out")
  {
  outname = (value == "console") ? "" : value;
  return 1;
  }
else if (option == "info")
  {
  bool bOK = LoadInfo(value, bProcInfo);
  if (bOK && !bSetDasm && !bProcInfo)
    saINames.push_back(value);
  return !!bOK;
  }
else if (option == "addr")
  showAddr = !!bnvalue;
else if (option == "hex")
  showHex = !!bnvalue;
else if (option == "asc")
  showAsc = !!bnvalue;
else if (pDasm)
  {
  // little special for f9dasm compatibility: "-no{option}" is interpreted
  // as "-option=" (which should default to off)
  if (option.substr(0, 2) == "no" &&
      pDasm->FindOption(option.substr(2)) >= 0)
    pDasm->SetOption(option.substr(2), "");
  else
    return pDasm->SetOption(option, value);
  }

return 0;
}

/*****************************************************************************/
/* ParseOptions : parse options                                              */
/*****************************************************************************/

void Application::ParseOptions
    (
    int argc,
    char* argv[],
    bool bSetDasm
    )
{
std::string curBegin, curEnd, curOff, curIlv("1");

// parse command line
for (int arg = 1; arg < argc; arg++)
  {
  if (argv[arg][0] == '-')
    {
    // allow -option:value or -option=value syntax, too
    std::string s(argv[arg] + 1);
    std::string::size_type x = s.find_first_of("=:");
    if (x != std::string::npos)
      ParseOption(s.substr(0, x), s.substr(x + 1),
                  bSetDasm, false);
    else
      arg += ParseOption(s, arg < argc - 1 ? argv[arg + 1] : "",
                         bSetDasm, false);
    }
  else if (!bSetDasm)
    {
    std::string name(argv[arg]);
    std::string silv("1");
    if (name.size() > 2)
      {
      // name can be name:interleave
      std::string::size_type idx = name.rfind(':');
      if (idx != std::string::npos)
        {
        silv = name.substr(idx + 1);
        if (silv.size() == 1 &&
            silv[0] >= '1' && silv[0] <= '8')
          name = name.substr(0, idx);
        else
          silv = "1";
        }
      }
    // in any case, the file relies on the current begin/end/offset settings
    // so their current setting must be preserved.
    std::string newBegin(pDasm->GetOption("begin"));
    if (newBegin != curBegin)
      {
      saFNames.push_back("-begin:"+newBegin);
      curBegin = newBegin;
      }
    std::string newEnd(pDasm->GetOption("end"));
    if (newEnd != curEnd)
      {
      saFNames.push_back("-end:"+newEnd);
      curEnd = newEnd;
      }
    std::string newOff(pDasm->GetOption("offset"));
    if (newOff != curOff)
      {
      saFNames.push_back("-offset:"+newOff);
      curOff = newOff;
      }
    if (silv != curIlv)
      {
      saFNames.push_back("-interleave:"+silv);
      curIlv = silv;
      }
    saFNames.push_back(name);
    }
  }
}

/*****************************************************************************/
/* ListOptions : lists the available options for the currently loaded dasm   */
/*****************************************************************************/

static void inline ListOptionLine
    (
    std::string name,
    std::string text,
    std::string current = ""
    )
{
printf("  %-13s ", name.c_str());
std::string::size_type idx = text.find('\t');
std::string optxt;
if (idx != std::string::npos)
  {
  optxt = text.substr(0, idx);
  text = text.substr(idx + 1);
  }
if (optxt.size())
  printf("%-10s ", optxt.c_str());
printf("%s", text.c_str());
if (current.size())
  printf("  (%s)", current.c_str());
printf("\n");
}

void Application::ListOptions()
{
printf("\nAvailable options");
#if 0
if (pDasm)
  printf(" for %s (%s)",
         Disassemblers[iDasm].name.c_str(),
         pDasm->GetName().c_str());
#endif
printf(":\n");
ListOptionLine("?|help", "This Help list");
ListOptionLine("out", "Output File name", outname.size() ? outname : "console");
ListOptionLine("dasm", "{code}\tDisassembler to use",
               pDasm ? Disassemblers[iDasm].name : std::string(""));

if (!pDasm)
  {
  ListDisassemblers();
  printf("\nTo show the available options for a disassembler, select one first\n"
         "and append -? to the command line\n");
  }
else
  {
  ListOptionLine("addr", "{off|on}\toutput address dump", showAddr ? "on" : "off");
  ListOptionLine("hex", "{off|on}\toutput hex dump", showHex ? "on" : "off");
  ListOptionLine("asc", "{off|on}\toutput ASCII rendering of code/data", showAsc ? "on" : "off");

  for (int i = 0; i < pDasm->GetOptionCount(); i++)
    ListOptionLine(pDasm->GetOptionName(i),
                   pDasm->GetOptionHelp(i),
                   pDasm->GetOption(i));
  }
printf("\nOptions can be given in the following formats:\n"
       "  -option value\n"
       "  -option=value\n"
       "  -option:value\n");
}


/*****************************************************************************/
/* Help : give help about the application                                    */
/*****************************************************************************/

int Application::Help(bool bQuit)
{
printf("Usage: dasmfw [-option]* [filename]*\n");

ListOptions();                          /* list options for selected dasm    */

if (bQuit) exit(1);                     /* hard exit                         */

return 1;
}

/*****************************************************************************/
/* DumpMem : dump disassembler's memory areas                                */
/*****************************************************************************/

#ifdef _DEBUG
void Application::DumpMem(bool bDataBus)
{
if (!pDasm) return;

std::string sComBlk = pDasm->GetOption("cchar") + " ";

int mac = pDasm->GetMemoryArrayCount(bDataBus);
int maac = pDasm->GetMemAttrArrayCount(bDataBus);
PrintLine(sComBlk +
          sformat("%s Bus: %d memory areas, %d memory attribute areas",
                  bDataBus ? "Data" : "Code", mac, maac));
int i;
for (i = 0; i < mac; i++)
  {
  TMemory<uint8_t, MemoryType> &area = pDasm->GetMemoryArray(i, bDataBus);
  PrintLine(sComBlk +
          sformat("Memory area %2d: start=%x size=%x", i,
                  area.GetStart(),
                  area.size()));
  }
MemAttributeHandler *pmah = pDasm->GetMemoryAttributeHandler(bDataBus);
for (i = 0; i < maac; i++)
  {
  TMemory<uint8_t, MemoryType> &area = pDasm->GetMemoryArray(i, bDataBus);
  PrintLine(sComBlk +
            sformat("Memory attribute area %2d: start=%x size=%x", i,
                    pmah->GetStart(i),
                    pmah->size(i)));
  }

int nLabels = pDasm->GetLabelCount(bDataBus);
for (i = 0; i < nLabels; i++)
  {
  Label *pLabel = pDasm->LabelAt(i, bDataBus);
  PrintLine(sComBlk +
          sformat("Label %4d: addr=%x type=%d \"%s\" %sused",
                  i, pLabel->GetAddress(), pLabel->GetType(),
                  pLabel->GetText().c_str(),
                  pLabel->IsUsed() ? "" : "un"));
  }

int nComments = GetCommentCount(false, bDataBus);
for (i = 0; i < nComments; i++)
  {
  Comment *pLabel = CommentAt(false, i, bDataBus);
  PrintLine(sComBlk +
          sformat("Comment %4d: before addr=%x \"%s\"",
                  i, pLabel->GetAddress(),
                  pLabel->GetText().c_str()));
  }
nComments = GetCommentCount(true, bDataBus);
for (i = 0; i < nComments; i++)
  {
  Comment *pLabel = CommentAt(true, i, bDataBus);
  PrintLine(sComBlk +
          sformat("Comment %4d: after  addr=%x \"%s\"",
                  i, pLabel->GetAddress(),
                  pLabel->GetText().c_str()));
  }
}
#endif
