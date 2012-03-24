------------------------------------------------------------------------------
-- DynASM ARM module.
--
-- Copyright (C) 2005-2012 Mike Pall. All rights reserved.
-- See dynasm.lua for full copyright notice.
------------------------------------------------------------------------------

-- Module information:
local _info = {
  arch =	"arm",
  description =	"DynASM ARM module",
  version =	"1.3.0",
  vernum =	 10300,
  release =	"2011-05-05",
  author =	"Mike Pall",
  license =	"MIT",
}

-- Exported glue functions for the arch-specific module.
local _M = { _info = _info }

-- Cache library functions.
local type, tonumber, pairs, ipairs = type, tonumber, pairs, ipairs
local assert, setmetatable, rawget = assert, setmetatable, rawget
local _s = string
local sub, format, byte, char = _s.sub, _s.format, _s.byte, _s.char
local match, gmatch, gsub = _s.match, _s.gmatch, _s.gsub
local concat, sort, insert = table.concat, table.sort, table.insert

-- Inherited tables and callbacks.
local g_opt, g_arch
local wline, werror, wfatal, wwarn

-- Action name list.
-- CHECK: Keep this in sync with the C code!
local action_names = {
  "STOP", "SECTION", "ESC", "REL_EXT",
  "ALIGN", "REL_LG", "LABEL_LG",
  "REL_PC", "LABEL_PC", "IMM", "IMM12", "IMM16", "IMML8", "IMML12",
}

-- Maximum number of section buffer positions for dasm_put().
-- CHECK: Keep this in sync with the C code!
local maxsecpos = 25 -- Keep this low, to avoid excessively long C lines.

-- Action name -> action number.
local map_action = {}
for n,name in ipairs(action_names) do
  map_action[name] = n-1
end

-- Action list buffer.
local actlist = {}

-- Argument list for next dasm_put(). Start with offset 0 into action list.
local actargs = { 0 }

-- Current number of section buffer positions for dasm_put().
local secpos = 1

------------------------------------------------------------------------------

-- Return 8 digit hex number.
local function tohex(x)
  return sub(format("%08x", x), -8) -- Avoid 64 bit portability problem in Lua.
end

-- Dump action names and numbers.
local function dumpactions(out)
  out:write("DynASM encoding engine action codes:\n")
  for n,name in ipairs(action_names) do
    local num = map_action[name]
    out:write(format("  %-10s %02X  %d\n", name, num, num))
  end
  out:write("\n")
end

-- Write action list buffer as a huge static C array.
local function writeactions(out, name)
  local nn = #actlist
  if nn == 0 then nn = 1; actlist[0] = map_action.STOP end
  out:write("static const unsigned int ", name, "[", nn, "] = {\n")
  for i = 1,nn-1 do
    assert(out:write("0x", tohex(actlist[i]), ",\n"))
  end
  assert(out:write("0x", tohex(actlist[nn]), "\n};\n\n"))
end

------------------------------------------------------------------------------

-- Add word to action list.
local function wputxw(n)
  assert(n >= 0 and n <= 0xffffffff and n % 1 == 0, "word out of range")
  actlist[#actlist+1] = n
end

-- Add action to list with optional arg. Advance buffer pos, too.
local function waction(action, val, a, num)
  local w = assert(map_action[action], "bad action name `"..action.."'")
  wputxw(w * 0x10000 + (val or 0))
  if a then actargs[#actargs+1] = a end
  if a or num then secpos = secpos + (num or 1) end
end

-- Flush action list (intervening C code or buffer pos overflow).
local function wflush(term)
  if #actlist == actargs[1] then return end -- Nothing to flush.
  if not term then waction("STOP") end -- Terminate action list.
  wline(format("dasm_put(Dst, %s);", concat(actargs, ", ")), true)
  actargs = { #actlist } -- Actionlist offset is 1st arg to next dasm_put().
  secpos = 1 -- The actionlist offset occupies a buffer position, too.
end

-- Put escaped word.
local function wputw(n)
  if n <= 0x000fffff then waction("ESC") end
  wputxw(n)
end

-- Reserve position for word.
local function wpos()
  local pos = #actlist+1
  actlist[pos] = ""
  return pos
end

-- Store word to reserved position.
local function wputpos(pos, n)
  assert(n >= 0 and n <= 0xffffffff and n % 1 == 0, "word out of range")
  if n <= 0x000fffff then
    insert(actlist, pos+1, n)
    n = map_action.ESC * 0x10000
  end
  actlist[pos] = n
end

------------------------------------------------------------------------------

-- Global label name -> global label number. With auto assignment on 1st use.
local next_global = 20
local map_global = setmetatable({}, { __index = function(t, name)
  if not match(name, "^[%a_][%w_]*$") then werror("bad global label") end
  local n = next_global
  if n > 2047 then werror("too many global labels") end
  next_global = n + 1
  t[name] = n
  return n
end})

-- Dump global labels.
local function dumpglobals(out, lvl)
  local t = {}
  for name, n in pairs(map_global) do t[n] = name end
  out:write("Global labels:\n")
  for i=20,next_global-1 do
    out:write(format("  %s\n", t[i]))
  end
  out:write("\n")
end

-- Write global label enum.
local function writeglobals(out, prefix)
  local t = {}
  for name, n in pairs(map_global) do t[n] = name end
  out:write("enum {\n")
  for i=20,next_global-1 do
    out:write("  ", prefix, t[i], ",\n")
  end
  out:write("  ", prefix, "_MAX\n};\n")
end

-- Write global label names.
local function writeglobalnames(out, name)
  local t = {}
  for name, n in pairs(map_global) do t[n] = name end
  out:write("static const char *const ", name, "[] = {\n")
  for i=20,next_global-1 do
    out:write("  \"", t[i], "\",\n")
  end
  out:write("  (const char *)0\n};\n")
end

------------------------------------------------------------------------------

-- Extern label name -> extern label number. With auto assignment on 1st use.
local next_extern = 0
local map_extern_ = {}
local map_extern = setmetatable({}, { __index = function(t, name)
  -- No restrictions on the name for now.
  local n = next_extern
  if n > 2047 then werror("too many extern labels") end
  next_extern = n + 1
  t[name] = n
  map_extern_[n] = name
  return n
end})

-- Dump extern labels.
local function dumpexterns(out, lvl)
  out:write("Extern labels:\n")
  for i=0,next_extern-1 do
    out:write(format("  %s\n", map_extern_[i]))
  end
  out:write("\n")
end

-- Write extern label names.
local function writeexternnames(out, name)
  out:write("static const char *const ", name, "[] = {\n")
  for i=0,next_extern-1 do
    out:write("  \"", map_extern_[i], "\",\n")
  end
  out:write("  (const char *)0\n};\n")
end

------------------------------------------------------------------------------

-- Arch-specific maps.

-- Ext. register name -> int. name.
local map_archdef = { sp = "r13", lr = "r14", pc = "r15", }

-- Int. register name -> ext. name.
local map_reg_rev = { r13 = "sp", r14 = "lr", r15 = "pc", }

local map_type = {}		-- Type name -> { ctype, reg }
local ctypenum = 0		-- Type number (for Dt... macros).

-- Reverse defines for registers.
function _M.revdef(s)
  return map_reg_rev[s] or s
end

local map_shift = { lsl = 0, lsr = 1, asr = 2, ror = 3, }

local map_cond = {
  eq = 0, ne = 1, cs = 2, cc = 3, mi = 4, pl = 5, vs = 6, vc = 7,
  hi = 8, ls = 9, ge = 10, lt = 11, gt = 12, le = 13, al = 14,
  hs = 2, lo = 3,
}

------------------------------------------------------------------------------

-- Template strings for ARM instructions.
local map_op = {
  -- Basic data processing instructions.
  and_3 = "e0000000DNPs",
  eor_3 = "e0200000DNPs",
  sub_3 = "e0400000DNPs",
  rsb_3 = "e0600000DNPs",
  add_3 = "e0800000DNPs",
  adc_3 = "e0a00000DNPs",
  sbc_3 = "e0c00000DNPs",
  rsc_3 = "e0e00000DNPs",
  tst_2 = "e1100000NP",
  teq_2 = "e1300000NP",
  cmp_2 = "e1500000NP",
  cmn_2 = "e1700000NP",
  orr_3 = "e1800000DNPs",
  mov_2 = "e1a00000DPs",
  bic_3 = "e1c00000DNPs",
  mvn_2 = "e1e00000DPs",

  and_4 = "e0000000DNMps",
  eor_4 = "e0200000DNMps",
  sub_4 = "e0400000DNMps",
  rsb_4 = "e0600000DNMps",
  add_4 = "e0800000DNMps",
  adc_4 = "e0a00000DNMps",
  sbc_4 = "e0c00000DNMps",
  rsc_4 = "e0e00000DNMps",
  tst_3 = "e1100000NMp",
  teq_3 = "e1300000NMp",
  cmp_3 = "e1500000NMp",
  cmn_3 = "e1700000NMp",
  orr_4 = "e1800000DNMps",
  mov_3 = "e1a00000DMps",
  bic_4 = "e1c00000DNMps",
  mvn_3 = "e1e00000DMps",

  lsl_3 = "e1a00000DMws",
  lsr_3 = "e1a00020DMws",
  asr_3 = "e1a00040DMws",
  ror_3 = "e1a00060DMws",
  rrx_2 = "e1a00060DMs",

  -- Multiply and multiply-accumulate.
  mul_3 = "e0000090NMSs",
  mla_4 = "e0200090NMSDs",
  umaal_4 = "e0400090DNMSs",	-- v6
  mls_4 = "e0600090DNMSs",	-- v6T2
  umull_4 = "e0800090DNMSs",
  umlal_4 = "e0a00090DNMSs",
  smull_4 = "e0c00090DNMSs",
  smlal_4 = "e0e00090DNMSs",

  -- Halfword multiply and multiply-accumulate.
  smlabb_4 = "e1000080NMSD",	-- v5TE
  smlatb_4 = "e10000a0NMSD",	-- v5TE
  smlabt_4 = "e10000c0NMSD",	-- v5TE
  smlatt_4 = "e10000e0NMSD",	-- v5TE
  smlawb_4 = "e1200080NMSD",	-- v5TE
  smulwb_3 = "e12000a0NMS",	-- v5TE
  smlawt_4 = "e12000c0NMSD",	-- v5TE
  smulwt_3 = "e12000e0NMS",	-- v5TE
  smlalbb_4 = "e1400080NMSD",	-- v5TE
  smlaltb_4 = "e14000a0NMSD",	-- v5TE
  smlalbt_4 = "e14000c0NMSD",	-- v5TE
  smlaltt_4 = "e14000e0NMSD",	-- v5TE
  smulbb_3 = "e1600080NMS",	-- v5TE
  smultb_3 = "e16000a0NMS",	-- v5TE
  smulbt_3 = "e16000c0NMS",	-- v5TE
  smultt_3 = "e16000e0NMS",	-- v5TE

  -- Miscellaneous data processing instructions.
  clz_2 = "e16f0f10DM", -- v5T
  rev_2 = "e6bf0f30DM", -- v6
  rev16_2 = "e6bf0fb0DM", -- v6
  revsh_2 = "e6ff0fb0DM", -- v6
  sel_3 = "e6800fb0DNM", -- v6
  usad8_3 = "e780f010NMS", -- v6
  usada8_4 = "e7800010NMSD", -- v6
  rbit_2 = "e6ff0f30DM", -- v6T2
  movw_2 = "e3000000DW", -- v6T2
  movt_2 = "e3400000DW", -- v6T2
  -- Note: the X encodes width-1, not width.
  sbfx_4 = "e7a00050DMvX", -- v6T2
  ubfx_4 = "e7e00050DMvX", -- v6T2
  -- Note: the X encodes the msb field, not the width.
  bfc_3 = "e7c0001fDvX", -- v6T2
  bfi_4 = "e7c00010DMvX", -- v6T2

  -- Packing and unpacking instructions.
  pkhbt_3 = "e6800010DNM", pkhbt_4 = "e6800010DNMv", -- v6
  pkhtb_3 = "e6800050DNM", pkhtb_4 = "e6800050DNMv", -- v6
  sxtab_3 = "e6a00070DNM", sxtab_4 = "e6a00070DNMv", -- v6
  sxtab16_3 = "e6800070DNM", sxtab16_4 = "e6800070DNMv", -- v6
  sxtah_3 = "e6b00070DNM", sxtah_4 = "e6b00070DNMv", -- v6
  sxtb_2 = "e6af0070DM", sxtb_3 = "e6af0070DMv", -- v6
  sxtb16_2 = "e68f0070DM", sxtb16_3 = "e68f0070DMv", -- v6
  sxth_2 = "e6bf0070DM", sxth_3 = "e6bf0070DMv", -- v6
  uxtab_3 = "e6e00070DNM", uxtab_4 = "e6e00070DNMv", -- v6
  uxtab16_3 = "e6c00070DNM", uxtab16_4 = "e6c00070DNMv", -- v6
  uxtah_3 = "e6f00070DNM", uxtah_4 = "e6f00070DNMv", -- v6
  uxtb_2 = "e6ef0070DM", uxtb_3 = "e6ef0070DMv", -- v6
  uxtb16_2 = "e6cf0070DM", uxtb16_3 = "e6cf0070DMv", -- v6
  uxth_2 = "e6ff0070DM", uxth_3 = "e6ff0070DMv", -- v6

  -- Saturating instructions.
  qadd_3 = "e1000050DMN",	-- v5TE
  qsub_3 = "e1200050DMN",	-- v5TE
  qdadd_3 = "e1400050DMN",	-- v5TE
  qdsub_3 = "e1600050DMN",	-- v5TE
  -- Note: the X for ssat* encodes sat_imm-1, not sat_imm.
  ssat_3 = "e6a00010DXM", ssat_4 = "e6a00010DXMp", -- v6
  usat_3 = "e6e00010DXM", usat_4 = "e6e00010DXMp", -- v6
  ssat16_3 = "e6a00f30DXM", -- v6
  usat16_3 = "e6e00f30DXM", -- v6

  -- Parallel addition and subtraction.
  sadd16_3 = "e6100f10DNM", -- v6
  sasx_3 = "e6100f30DNM", -- v6
  ssax_3 = "e6100f50DNM", -- v6
  ssub16_3 = "e6100f70DNM", -- v6
  sadd8_3 = "e6100f90DNM", -- v6
  ssub8_3 = "e6100ff0DNM", -- v6
  qadd16_3 = "e6200f10DNM", -- v6
  qasx_3 = "e6200f30DNM", -- v6
  qsax_3 = "e6200f50DNM", -- v6
  qsub16_3 = "e6200f70DNM", -- v6
  qadd8_3 = "e6200f90DNM", -- v6
  qsub8_3 = "e6200ff0DNM", -- v6
  shadd16_3 = "e6300f10DNM", -- v6
  shasx_3 = "e6300f30DNM", -- v6
  shsax_3 = "e6300f50DNM", -- v6
  shsub16_3 = "e6300f70DNM", -- v6
  shadd8_3 = "e6300f90DNM", -- v6
  shsub8_3 = "e6300ff0DNM", -- v6
  uadd16_3 = "e6500f10DNM", -- v6
  uasx_3 = "e6500f30DNM", -- v6
  usax_3 = "e6500f50DNM", -- v6
  usub16_3 = "e6500f70DNM", -- v6
  uadd8_3 = "e6500f90DNM", -- v6
  usub8_3 = "e6500ff0DNM", -- v6
  uqadd16_3 = "e6600f10DNM", -- v6
  uqasx_3 = "e6600f30DNM", -- v6
  uqsax_3 = "e6600f50DNM", -- v6
  uqsub16_3 = "e6600f70DNM", -- v6
  uqadd8_3 = "e6600f90DNM", -- v6
  uqsub8_3 = "e6600ff0DNM", -- v6
  uhadd16_3 = "e6700f10DNM", -- v6
  uhasx_3 = "e6700f30DNM", -- v6
  uhsax_3 = "e6700f50DNM", -- v6
  uhsub16_3 = "e6700f70DNM", -- v6
  uhadd8_3 = "e6700f90DNM", -- v6
  uhsub8_3 = "e6700ff0DNM", -- v6

  -- Load/store instructions.
  str_2 = "e4000000DL", str_3 = "e4000000DL", str_4 = "e4000000DL",
  strb_2 = "e4400000DL", strb_3 = "e4400000DL", strb_4 = "e4400000DL",
  ldr_2 = "e4100000DL", ldr_3 = "e4100000DL", ldr_4 = "e4100000DL",
  ldrb_2 = "e4500000DL", ldrb_3 = "e4500000DL", ldrb_4 = "e4500000DL",
  strh_2 = "e00000b0DL", strh_3 = "e00000b0DL",
  ldrh_2 = "e01000b0DL", ldrh_3 = "e01000b0DL",
  ldrd_2 = "e00000d0DL", ldrd_3 = "e00000d0DL", -- v5TE
  ldrsb_2 = "e01000d0DL", ldrsb_3 = "e01000d0DL",
  strd_2 = "e00000f0DL", strd_3 = "e00000f0DL", -- v5TE
  ldrsh_2 = "e01000f0DL", ldrsh_3 = "e01000f0DL",

  ldm_2 = "e8900000nR", ldmia_2 = "e8900000nR", ldmfd_2 = "e8900000nR",
  ldmda_2 = "e8100000nR", ldmfa_2 = "e8100000nR",
  ldmdb_2 = "e9100000nR", ldmea_2 = "e9100000nR",
  ldmib_2 = "e9900000nR", ldmed_2 = "e9900000nR",
  stm_2 = "e8800000nR", stmia_2 = "e8800000nR", stmfd_2 = "e8800000nR",
  stmda_2 = "e8000000nR", stmfa_2 = "e8000000nR",
  stmdb_2 = "e9000000nR", stmea_2 = "e9000000nR",
  stmib_2 = "e9800000nR", stmed_2 = "e9800000nR",
  pop_1 = "e8bd0000R", push_1 = "e92d0000R",

  -- Branch instructions.
  b_1 = "ea000000B",
  bl_1 = "eb000000B",
  blx_1 = "e12fff30C",
  bx_1 = "e12fff10M",

  -- Miscellaneous instructions.
  nop_0 = "e1a00000",
  mrs_1 = "e10f0000D",
  bkpt_1 = "e1200070K", -- v5T
  svc_1 = "ef000000T", swi_1 = "ef000000T",
  ud_0 = "e7f001f0",

  -- NYI: Advanced SIMD and VFP instructions.

  -- NYI instructions, since I have no need for them right now:
  -- swp, swpb, strex, ldrex, strexd, ldrexd, strexb, ldrexb, strexh, ldrexh
  -- msr, nopv6, yield, wfe, wfi, sev, dbg, bxj, smc, srs, rfe
  -- cps, setend, pli, pld, pldw, clrex, dsb, dmb, isb
  -- stc, ldc, mcr, mcr2, mrc, mrc2, mcrr, mcrr2, mrrc, mrrc2, cdp, cdp2
}

-- Add mnemonics for "s" variants.
do
  local t = {}
  for k,v in pairs(map_op) do
    if sub(v, -1) == "s" then
      local v2 = sub(v, 1, 2)..char(byte(v, 3)+1)..sub(v, 4, -2)
      t[sub(k, 1, -3).."s"..sub(k, -2)] = v2
    end
  end
  for k,v in pairs(t) do
    map_op[k] = v
  end
end

------------------------------------------------------------------------------

local function parse_gpr(expr)
  local tname, ovreg = match(expr, "^([%w_]+):(r1?[0-9])$")
  local tp = map_type[tname or expr]
  if tp then
    local reg = ovreg or tp.reg
    if not reg then
      werror("type `"..(tname or expr).."' needs a register override")
    end
    expr = reg
  end
  local r = match(expr, "^r(1?[0-9])$")
  if r then
    r = tonumber(r)
    if r <= 15 then return r, tp end
  end
  werror("bad register name `"..expr.."'")
end

local function parse_gpr_pm(expr)
  local pm, expr2 = match(expr, "^([+-]?)(.*)$")
  return parse_gpr(expr2), (pm == "-")
end

local function parse_reglist(reglist)
  reglist = match(reglist, "^{%s*([^}]*)}$")
  if not reglist then werror("register list expected") end
  local rr = 0
  for p in gmatch(reglist..",", "%s*([^,]*),") do
    local rbit = 2^parse_gpr(gsub(p, "%s+$", ""))
    if ((rr - (rr % rbit)) / rbit) % 2 ~= 0 then
      werror("duplicate register `"..p.."'")
    end
    rr = rr + rbit
  end
  return rr
end

local function parse_imm(imm, bits, shift, scale, signed)
  imm = match(imm, "^#(.*)$")
  if not imm then werror("expected immediate operand") end
  local n = tonumber(imm)
  if n then
    if n % 2^scale == 0 then
      n = n / 2^scale
      if signed then
	if n >= 0 then
	  if n < 2^(bits-1) then return n*2^shift end
	else
	  if n >= -(2^(bits-1))-1 then return (n+2^bits)*2^shift end
	end
      else
	if n >= 0 and n <= 2^bits-1 then return n*2^shift end
      end
    end
    werror("out of range immediate `"..imm.."'")
  else
    waction("IMM", (signed and 32768 or 0)+scale*1024+bits*32+shift, imm)
    return 0
  end
end

local function parse_imm12(imm)
  local n = tonumber(imm)
  if n then
    local m = n
    for i=0,-15,-1 do
      if m >= 0 and m <= 255 and n % 1 == 0 then return m + (i%16) * 256 end
      local t = m % 4
      m = (m - t) / 4 + t * 2^30
    end
    werror("out of range immediate `"..imm.."'")
  else
    waction("IMM12", 0, imm)
    return 0
  end
end

local function parse_imm16(imm)
  imm = match(imm, "^#(.*)$")
  if not imm then werror("expected immediate operand") end
  local n = tonumber(imm)
  if n then
    if n >= 0 and n <= 65535 and n % 1 == 0 then
      local t = n % 4096
      return (n - t) * 16 + t
    end
    werror("out of range immediate `"..imm.."'")
  else
    waction("IMM16", 32*16, imm)
    return 0
  end
end

local function parse_imm_load(imm, ext)
  local n = tonumber(imm)
  if n then
    if ext then
      if n >= -255 and n <= 255 then
	local up = 0x00800000
	if n < 0 then n = -n; up = 0 end
	return (n-(n%16))*16+(n%16) + up
      end
    else
      if n >= -4095 and n <= 4095 then
	if n >= 0 then return n+0x00800000 end
	return -n
      end
    end
    werror("out of range immediate `"..imm.."'")
  else
    waction(ext and "IMML8" or "IMML12", 32768 + 32*(ext and 8 or 12), imm)
    return 0
  end
end

local function parse_shift(shift, gprok)
  if shift == "rrx" then
    return 3 * 32
  else
    local s, s2 = match(shift, "^(%S+)%s*(.*)$")
    s = map_shift[s]
    if not s then werror("expected shift operand") end
    if sub(s2, 1, 1) == "#" then
      return parse_imm(s2, 5, 7, 0, false) + s * 32
    else
      if not gprok then werror("expected immediate shift operand") end
      return parse_gpr(s2) * 256 + s * 32 + 16
    end
  end
end

local function parse_label(label, def)
  local prefix = sub(label, 1, 2)
  -- =>label (pc label reference)
  if prefix == "=>" then
    return "PC", 0, sub(label, 3)
  end
  -- ->name (global label reference)
  if prefix == "->" then
    return "LG", map_global[sub(label, 3)]
  end
  if def then
    -- [1-9] (local label definition)
    if match(label, "^[1-9]$") then
      return "LG", 10+tonumber(label)
    end
  else
    -- [<>][1-9] (local label reference)
    local dir, lnum = match(label, "^([<>])([1-9])$")
    if dir then -- Fwd: 1-9, Bkwd: 11-19.
      return "LG", lnum + (dir == ">" and 0 or 10)
    end
    -- extern label (extern label reference)
    local extname = match(label, "^extern%s+(%S+)$")
    if extname then
      return "EXT", map_extern[extname]
    end
  end
  werror("bad label `"..label.."'")
end

local function parse_load(params, nparams, n, op)
  local oplo = op % 256
  local ext, ldrd = (oplo ~= 0), (oplo == 208)
  local d
  if (ldrd or oplo == 240) then
    d = ((op - (op % 4096)) / 4096) % 16
    if d % 2 ~= 0 then werror("odd destination register") end
  end
  local pn = params[n]
  local p1, wb = match(pn, "^%[%s*(.-)%s*%](!?)$")
  local p2 = params[n+1]
  if not p1 then
    if not p2 then
      if match(pn, "^[<>=%-]") or match(pn, "^extern%s+") then
	local mode, n, s = parse_label(pn, false)
	waction("REL_"..mode, n + (ext and 0x1800 or 0x0800), s, 1)
	return op + 15 * 65536 + 0x01000000 + (ext and 0x00400000 or 0)
      end
      local reg, tailr = match(pn, "^([%w_:]+)%s*(.*)$")
      if reg and tailr ~= "" then
	local d, tp = parse_gpr(reg)
	if tp then
	  waction(ext and "IMML8" or "IMML12", 32768 + 32*(ext and 8 or 12),
		  format(tp.ctypefmt, tailr))
	  return op + d * 65536 + 0x01000000 + (ext and 0x00400000 or 0)
	end
      end
    end
    werror("expected address operand")
  end
  if wb == "!" then op = op + 0x00200000 end
  if p2 then
    if wb == "!" then werror("bad use of '!'") end
    local p3 = params[n+2]
    op = op + parse_gpr(p1) * 65536
    local imm = match(p2, "^#(.*)$")
    if imm then
      local m = parse_imm_load(imm, ext)
      if p3 then werror("too many parameters") end
      op = op + m + (ext and 0x00400000 or 0)
    else
      local m, neg = parse_gpr_pm(p2)
      if ldrd and (m == d or m-1 == d) then werror("register conflict") end
      op = op + m + (neg and 0 or 0x00800000) + (ext and 0 or 0x02000000)
      if p3 then op = op + parse_shift(p3) end
    end
  else
    local p1a, p2 = match(p1, "^([^,%s]*)%s*(.*)$")
    op = op + parse_gpr(p1a) * 65536 + 0x01000000
    if p2 ~= "" then
      local imm = match(p2, "^,%s*#(.*)$")
      if imm then
	local m = parse_imm_load(imm, ext)
	op = op + m + (ext and 0x00400000 or 0)
      else
	local p2a, p3 = match(p2, "^,%s*([^,%s]*)%s*,?%s*(.*)$")
	local m, neg = parse_gpr_pm(p2a)
	if ldrd and (m == d or m-1 == d) then werror("register conflict") end
	op = op + m + (neg and 0 or 0x00800000) + (ext and 0 or 0x02000000)
	if p3 ~= "" then
	  if ext then werror("too many parameters") end
	  op = op + parse_shift(p3)
	end
      end
    else
      if wb == "!" then werror("bad use of '!'") end
      op = op + (ext and 0x00c00000 or 0x00800000)
    end
  end
  return op
end

------------------------------------------------------------------------------

-- Handle opcodes defined with template strings.
map_op[".template__"] = function(params, template, nparams)
  if not params then return sub(template, 9) end
  local op = tonumber(sub(template, 1, 8), 16)
  local n = 1

  -- Limit number of section buffer positions used by a single dasm_put().
  -- A single opcode needs a maximum of 3 positions.
  if secpos+3 > maxsecpos then wflush() end
  local pos = wpos()

  -- Process each character.
  for p in gmatch(sub(template, 9), ".") do
    if p == "D" then
      op = op + parse_gpr(params[n]) * 4096; n = n + 1
    elseif p == "N" then
      op = op + parse_gpr(params[n]) * 65536; n = n + 1
    elseif p == "S" then
      op = op + parse_gpr(params[n]) * 256; n = n + 1
    elseif p == "M" then
      op = op + parse_gpr(params[n]); n = n + 1
    elseif p == "P" then
      local imm = match(params[n], "^#(.*)$")
      if imm then
	op = op + parse_imm12(imm) + 0x02000000
      else
	op = op + parse_gpr(params[n])
      end
      n = n + 1
    elseif p == "p" then
      op = op + parse_shift(params[n], true); n = n + 1
    elseif p == "L" then
      op = parse_load(params, nparams, n, op)
    elseif p == "B" then
      local mode, n, s = parse_label(params[n], false)
      waction("REL_"..mode, n, s, 1)
    elseif p == "C" then -- blx gpr vs. blx label.
      local p = params[n]
      if match(p, "^([%w_]+):(r1?[0-9])$") or match(p, "^r(1?[0-9])$") then
	op = op + parse_gpr(p)
      else
	if op < 0xe0000000 then werror("unconditional instruction") end
	local mode, n, s = parse_label(p, false)
	waction("REL_"..mode, n, s, 1)
	op = 0xfa000000
      end
    elseif p == "n" then
      local r, wb = match(params[n], "^([^!]*)(!?)$")
      op = op + parse_gpr(r) * 65536 + (wb == "!" and 0x00200000 or 0)
      n = n + 1
    elseif p == "R" then
      op = op + parse_reglist(params[n]); n = n + 1
    elseif p == "W" then
      op = op + parse_imm16(params[n]); n = n + 1
    elseif p == "v" then
      op = op + parse_imm(params[n], 5, 7, 0, false); n = n + 1
    elseif p == "w" then
      local imm = match(params[n], "^#(.*)$")
      if imm then
	op = op + parse_imm(params[n], 5, 7, 0, false); n = n + 1
      else
	op = op + parse_gpr(params[n]) * 256 + 16
      end
    elseif p == "X" then
      op = op + parse_imm(params[n], 5, 16, 0, false); n = n + 1
    elseif p == "K" then
      local imm = tonumber(match(params[n], "^#(.*)$")); n = n + 1
      if not imm or imm % 1 ~= 0 or imm < 0 or imm > 0xffff then
	werror("bad immediate operand")
      end
      local t = imm % 16
      op = op + (imm - t) * 16 + t
    elseif p == "T" then
      op = op + parse_imm(params[n], 24, 0, 0, false); n = n + 1
    elseif p == "s" then
      -- Ignored.
    else
      assert(false)
    end
  end
  wputpos(pos, op)
end

------------------------------------------------------------------------------

-- Pseudo-opcode to mark the position where the action list is to be emitted.
map_op[".actionlist_1"] = function(params)
  if not params then return "cvar" end
  local name = params[1] -- No syntax check. You get to keep the pieces.
  wline(function(out) writeactions(out, name) end)
end

-- Pseudo-opcode to mark the position where the global enum is to be emitted.
map_op[".globals_1"] = function(params)
  if not params then return "prefix" end
  local prefix = params[1] -- No syntax check. You get to keep the pieces.
  wline(function(out) writeglobals(out, prefix) end)
end

-- Pseudo-opcode to mark the position where the global names are to be emitted.
map_op[".globalnames_1"] = function(params)
  if not params then return "cvar" end
  local name = params[1] -- No syntax check. You get to keep the pieces.
  wline(function(out) writeglobalnames(out, name) end)
end

-- Pseudo-opcode to mark the position where the extern names are to be emitted.
map_op[".externnames_1"] = function(params)
  if not params then return "cvar" end
  local name = params[1] -- No syntax check. You get to keep the pieces.
  wline(function(out) writeexternnames(out, name) end)
end

------------------------------------------------------------------------------

-- Label pseudo-opcode (converted from trailing colon form).
map_op[".label_1"] = function(params)
  if not params then return "[1-9] | ->global | =>pcexpr" end
  if secpos+1 > maxsecpos then wflush() end
  local mode, n, s = parse_label(params[1], true)
  if mode == "EXT" then werror("bad label definition") end
  waction("LABEL_"..mode, n, s, 1)
end

------------------------------------------------------------------------------

-- Pseudo-opcodes for data storage.
map_op[".long_*"] = function(params)
  if not params then return "imm..." end
  for _,p in ipairs(params) do
    local n = tonumber(p)
    if not n then werror("bad immediate `"..p.."'") end
    if n < 0 then n = n + 2^32 end
    wputw(n)
    if secpos+2 > maxsecpos then wflush() end
  end
end

-- Alignment pseudo-opcode.
map_op[".align_1"] = function(params)
  if not params then return "numpow2" end
  if secpos+1 > maxsecpos then wflush() end
  local align = tonumber(params[1])
  if align then
    local x = align
    -- Must be a power of 2 in the range (2 ... 256).
    for i=1,8 do
      x = x / 2
      if x == 1 then
	waction("ALIGN", align-1, nil, 1) -- Action byte is 2**n-1.
	return
      end
    end
  end
  werror("bad alignment")
end

------------------------------------------------------------------------------

-- Pseudo-opcode for (primitive) type definitions (map to C types).
map_op[".type_3"] = function(params, nparams)
  if not params then
    return nparams == 2 and "name, ctype" or "name, ctype, reg"
  end
  local name, ctype, reg = params[1], params[2], params[3]
  if not match(name, "^[%a_][%w_]*$") then
    werror("bad type name `"..name.."'")
  end
  local tp = map_type[name]
  if tp then
    werror("duplicate type `"..name.."'")
  end
  -- Add #type to defines. A bit unclean to put it in map_archdef.
  map_archdef["#"..name] = "sizeof("..ctype..")"
  -- Add new type and emit shortcut define.
  local num = ctypenum + 1
  map_type[name] = {
    ctype = ctype,
    ctypefmt = format("Dt%X(%%s)", num),
    reg = reg,
  }
  wline(format("#define Dt%X(_V) (int)(ptrdiff_t)&(((%s *)0)_V)", num, ctype))
  ctypenum = num
end
map_op[".type_2"] = map_op[".type_3"]

-- Dump type definitions.
local function dumptypes(out, lvl)
  local t = {}
  for name in pairs(map_type) do t[#t+1] = name end
  sort(t)
  out:write("Type definitions:\n")
  for _,name in ipairs(t) do
    local tp = map_type[name]
    local reg = tp.reg or ""
    out:write(format("  %-20s %-20s %s\n", name, tp.ctype, reg))
  end
  out:write("\n")
end

------------------------------------------------------------------------------

-- Set the current section.
function _M.section(num)
  waction("SECTION", num)
  wflush(true) -- SECTION is a terminal action.
end

------------------------------------------------------------------------------

-- Dump architecture description.
function _M.dumparch(out)
  out:write(format("DynASM %s version %s, released %s\n\n",
    _info.arch, _info.version, _info.release))
  dumpactions(out)
end

-- Dump all user defined elements.
function _M.dumpdef(out, lvl)
  dumptypes(out, lvl)
  dumpglobals(out, lvl)
  dumpexterns(out, lvl)
end

------------------------------------------------------------------------------

-- Pass callbacks from/to the DynASM core.
function _M.passcb(wl, we, wf, ww)
  wline, werror, wfatal, wwarn = wl, we, wf, ww
  return wflush
end

-- Setup the arch-specific module.
function _M.setup(arch, opt)
  g_arch, g_opt = arch, opt
end

-- Merge the core maps and the arch-specific maps.
function _M.mergemaps(map_coreop, map_def)
  setmetatable(map_op, { __index = function(t, k)
    local v = map_coreop[k]
    if v then return v end
    local cc = sub(k, -4, -3)
    local cv = map_cond[cc]
    if cv then
      local v = rawget(t, sub(k, 1, -5)..sub(k, -2))
      if type(v) == "string" then return format("%x%s", cv, sub(v, 2)) end
    end
  end })
  setmetatable(map_def, { __index = map_archdef })
  return map_op, map_def
end

return _M

------------------------------------------------------------------------------

