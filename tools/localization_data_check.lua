-- Independent semantic comparison through the real Lua parser.
-- Usage: texlua tools/localization_data_check.lua DECODED_SCRIPT_DIR OVERLAY_SCRIPT_DIR
assert(arg[1] and arg[2], 'Pass decoded source and translated script directories')
local source=arg[1]..'/'
local target=arg[2]..'/'
local fields={bikes={tech_info=true},tracks={location_name=true,gp_name=true},upgrades={name=true,description=true},unlockables={description=true},credits={text=true}}
local checks,changes=0,0
local function walk(a,b,allowed,path)
 assert(type(a)==type(b),'Type changed at '..path)
 if type(a)~='table' then assert(a==b,'Data changed at '..path);checks=checks+1;return end
 for key,value in pairs(a) do
  assert(b[key]~=nil or value==nil,'Missing key at '..path..'.'..tostring(key))
  if allowed[key] and type(value)=='string' then
   assert(type(b[key])=='string','Display field type changed')
   if value~=b[key] then changes=changes+1 end
  else walk(value,b[key],allowed,path..'.'..tostring(key)) end
 end
 for key in pairs(b) do assert(a[key]~=nil,'New key at '..path..'.'..tostring(key)) end
end
for name,allowed in pairs(fields) do
 local a,b={},{}
 assert(loadfile(source..name..'.english.lua','t',a))()
 assert(loadfile(target..name..'.english.lua','t',b))()
 walk(a,b,allowed,name)
end
print(checks..' independent Lua data checks passed; '..changes..' display fields translated; physics, IDs, paths and staff data unchanged')
