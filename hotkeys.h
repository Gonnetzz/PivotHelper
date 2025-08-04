#pragma once

namespace Hotkeys {
    enum class Action {
        None,
        New,
        Open,
        Save,
        SaveAs,
        Quit
    };

    Action Process();
}