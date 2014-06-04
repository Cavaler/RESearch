local function RESearchExists ()
  return Plugin.Exist("F250C12A-78E2-4ABC-A784-3FDD3156E415")
end

local function CallRS(...)
  return Plugin.SyncCall("F250C12A-78E2-4ABC-A784-3FDD3156E415", ...)
end

local function CallRSS(...)
  return Plugin.Call("F250C12A-78E2-4ABC-A784-3FDD3156E415", ...)
end

_G.research = {}

function research.match(n)
  return CallRSS("Script", "match", n)
end

function research.named(n)
  return CallRSS("Script", "named", n)
end

function research.eol()
  return CallRSS("Script", "eol")
end

function research.l()
  return CallRSS("Script", "l")
end

function research.n()
  return CallRSS("Script", "n")
end

function research.s()
  return CallRSS("Script", "s")
end

function research.r()
  return CallRSS("Script", "r")
end

function research.init(n, v)
  CallRSS("Script", "init", n, v)
end

function research.store(n, v)
  CallRSS("Script", "store", n, v)
end

function research.skip()
  CallRSS("Script", "skip")
end

Macro {
  description=""; area="Shell"; key="AltF7"; flags=""; condition=RESearchExists;
  action=function() CallRS("Search") end;
}

Macro {
  description=""; area="Shell"; key="ShiftF7"; flags=""; condition=RESearchExists;
  action=function() CallRS("Replace") end;
}

Macro {
  description=""; area="Shell"; key="CtrlAltF7"; flags=""; condition=RESearchExists;
  action=function() CallRS("Grep") end;
}

Macro {
  description=""; area="Shell"; key="AltF6"; flags=""; condition=RESearchExists;
  action=function() CallRS("RenameSelected") end;
}

Macro {
  description=""; area="Shell"; key="CtrlAltF6"; flags=""; condition=RESearchExists;
  action=function() CallRS("Rename") end;
}

Macro {
  description=""; area="Shell"; key="AltAdd"; flags=""; condition=RESearchExists;
  action=function() CallRS("Select") end;
}

Macro {
  description=""; area="Shell"; key="AltSubtract"; flags=""; condition=RESearchExists;
  action=function() CallRS("Unselect") end;
}

Macro {
  description=""; area="Shell"; key="AltMultiply"; flags=""; condition=RESearchExists;
  action=function() CallRS("FlipSelection") end;
}



Macro {
  description=""; area="Editor"; key="F7"; flags=""; condition=RESearchExists;
  action=function() CallRS("Search") end;
}

Macro {
  description=""; area="Editor"; key="CtrlF7"; flags=""; condition=RESearchExists;
  action=function() CallRS("Replace") end;
}

Macro {
  description=""; area="Editor"; key="ShiftF7"; flags=""; condition=RESearchExists;
  action=function() CallRS("SRAgain") end;
}

Macro {
  description=""; area="Editor"; key="AltF7"; flags=""; condition=RESearchExists;
  action=function() CallRS("Filter") end;
}



Macro {
  description=""; area="Viewer"; key="F7"; flags=""; condition=RESearchExists;
  action=function() CallRS("Search") end;
}

Macro {
  description=""; area="Viewer"; key="ShiftF7"; flags=""; condition=RESearchExists;
  action=function() CallRS("SRAgain") end;
}
