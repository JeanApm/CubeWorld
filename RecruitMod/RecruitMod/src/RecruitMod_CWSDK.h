#pragma once

namespace RecruitMod {
    void Init();
    void Tick();       // ~10–20 Hz
    void CmdRecruit(); // /recruit
    void CmdDismiss(); // /dismiss
    void CmdParty();   // /party
}
