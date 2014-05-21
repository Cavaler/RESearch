local function RESearchExists ()
  return Plugin.Exist("F250C12A-78E2-4ABC-A784-3FDD3156E415")
end

local function CallRS1(p)
  mmode(3,1)
  return Plugin.SyncCall("F250C12A-78E2-4ABC-A784-3FDD3156E415", p)
end

local function CallRS2(p1, p2)
  mmode(3,1)
  return Plugin.SyncCall("F250C12A-78E2-4ABC-A784-3FDD3156E415", p1, p2)
end

local function CallRS3(p1, p2, p3)
  mmode(3,1)
  return Plugin.SyncCall("F250C12A-78E2-4ABC-A784-3FDD3156E415", p1, p2, p3)
end

local function CallRS4(p1, p2, p3, p4)
  mmode(3,1)
  return Plugin.SyncCall("F250C12A-78E2-4ABC-A784-3FDD3156E415", p1, p2, p3, p4)
end

function _G.match(n)
  return CallRS3("Script", "match", n)
end

function _G.named(n)
  return CallRS3("Script", "named", n)
end

function _G.eol()
  return CallRS2("Script", "eol")
end

function _G.l()
  return CallRS2("Script", "l")
end

function _G.n()
  return CallRS2("Script", "n")
end

function _G.s()
  return CallRS2("Script", "s")
end

function _G.r()
  return CallRS2("Script", "r")
end

function _G.init(n, v)
  return CallRS4("Script", "init", n, v)
end

function _G.store(n, v)
  return CallRS4("Script", "store", n, v)
end

Macro {
  description=""; area="Shell"; key="AltF7"; flags=""; condition=RESearchExists;
--  action=function() CallRS1("Search") end;
  action=function() Keys("F11 s s") end;
}

Macro {
  description=""; area="Shell"; key="ShiftF7"; flags=""; condition=RESearchExists;
--  action=function() CallRS1("Replace") end;
  action=function() Keys("F11 s r") end;
}

Macro {
  description=""; area="Shell"; key="CtrlAltF7"; flags=""; condition=RESearchExists;
  action=function() CallRS1("Grep") end;
}

Macro {
  description=""; area="Shell"; key="AltF6"; flags=""; condition=RESearchExists;
  action=function() CallRS1("RenameSelected") end;
}

Macro {
  description=""; area="Shell"; key="CtrlAltF6"; flags=""; condition=RESearchExists;
  action=function() CallRS1("Rename") end;
}

Macro {
  description=""; area="Shell"; key="AltAdd"; flags=""; condition=RESearchExists;
  action=function() CallRS1("Select") end;
}

Macro {
  description=""; area="Shell"; key="AltSubtract"; flags=""; condition=RESearchExists;
  action=function() CallRS1("Unselect") end;
}

Macro {
  description=""; area="Shell"; key="AltMultiply"; flags=""; condition=RESearchExists;
  action=function() CallRS1("FlipSelection") end;
}




Macro {
  description=""; area="Editor"; key="F7"; flags=""; condition=RESearchExists;
  action=function() CallRS1("Search") end;
}

Macro {
  description=""; area="Editor"; key="CtrlF7"; flags=""; condition=RESearchExists;
  action=function() CallRS1("Replace") end;
}

Macro {
  description=""; area="Editor"; key="ShiftF7"; flags=""; condition=RESearchExists;
  action=function() CallRS1("SRAgain") end;
}

Macro {
  description=""; area="Editor"; key="AltF7"; flags=""; condition=RESearchExists;
  action=function() CallRS1("Filter") end;
}



Macro {
  description=""; area="Viewer"; key="F7"; flags=""; condition=RESearchExists;
  action=function() CallRS1("Search") end;
}

Macro {
  description=""; area="Viewer"; key="ShiftF7"; flags=""; condition=RESearchExists;
  action=function() CallRS1("SRAgain") end;
}
