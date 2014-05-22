local function RESearchExists ()
  return Plugin.Exist("F250C12A-78E2-4ABC-A784-3FDD3156E415")
end

local function CallRS(...)
  return Plugin.SyncCall("F250C12A-78E2-4ABC-A784-3FDD3156E415", ...)
end

_G.research = {}

function research.match(n)
  return CallRS("Script", "match", n)
end

function research.named(n)
  return CallRS("Script", "named", n)
end

function research.eol()
  return CallRS("Script", "eol")
end

function research.l()
  return CallRS("Script", "l")
end

function research.n()
  return CallRS("Script", "n")
end

function research.s()
  return CallRS("Script", "s")
end

function research.r()
  return CallRS("Script", "r")
end

function research.init(n, v)
  CallRS("Script", "init", n, v)
end

function research.store(n, v)
  CallRS("Script", "store", n, v)
end

function research.skip()
  CallRS("Script", "skip")
end

Macro {
  description=""; area="Shell"; key="AltF7"; flags=""; condition=RESearchExists;
--  action=function() CallRS("Search") end;
  action=function() Keys("F11 s s") end;
}

Macro {
  description=""; area="Shell"; key="ShiftF7"; flags=""; condition=RESearchExists;
--  action=function() CallRS("Replace") end;
  action=function() Keys("F11 s r") end;
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
