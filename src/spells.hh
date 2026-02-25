#ifndef TIBIA_SPELLS_HH_
#define TIBIA_SPELLS_HH_ 1

#include "common.hh"        // for MAX_SPELL_SYLLABLES etc
#include <cstdint>

// A data-driven description of a spell.  The fields are intentionally
// simple so that they can be manipulated from a human-readable
// configuration file (`dat/spells.dat`).  Most of the original Ot client
// engine used a huge switch statement with hardcoded constants; here we
// keep the same structure but make it mutable at runtime.
struct TSpellList {
    uint8_t Syllable[MAX_SPELL_SYLLABLES];
    uint8_t RuneGr;
    uint8_t RuneNr;
    const char *Comment;
    uint16_t Level;
    uint16_t RuneLevel;
    uint16_t Flags;
    int Mana;
    int SoulPoints;
    int Amount;

    // Generic parameters that are interpreted differently by each
    // handler.  Some built‑in spells use them for damage ranges,
    // durations, radii, etc.  They are not understood by the loader;
    // value semantics are left to the spell implementation.
    int Param1;
    int Param2;
    int Param3;
    int Param4;

    // Visual effect (EFFECT_*) and missile animation (ANIMATION_*).
    // When zero, the engine will fall back to the hardcoded default.
    int Effect;
    int Animation;

    // If non‑zero, this spell inherits any field that is not explicitly
    // set from the referenced spell number.  The value is resolved
    // once during initialization so subsequent lookups are inexpensive.
    int Template;
};

constexpr int MAX_SPELLS = 256;

// public access for code that still references the old global array.  We
// prefer the helper functions above, but keeping the symbol around makes
// the migration incremental and avoids editing hundreds of call sites.
extern TSpellList SpellList[MAX_SPELLS];

// Initialise the global spell list by reading `dat/spells.dat`.  This
// must be called before any other spells-related function; it is
// triggered by `InitMagic()` during server startup.  Repeated calls
// re-read the file and re-apply templates, so it can also be used for
// a manual reload command.
void SpellsInit(void);

// Convenience wrapper returning `true` iff the number is in the valid
// range and has an entry (after initialization).  Using this instead of
// a raw array check makes the rest of the code easier to migrate.
bool SpellsValid(int SpellNr);

// Returns a pointer to the spell data or `nullptr` if the index is
// invalid.  The pointer remains owned by the spells module and must not
// be freed or modified by callers.
const TSpellList *SpellsGet(int SpellNr);

// Visual/animation helpers identical to the ones we previously had in
// `magic.cc`.  They live here so any consumer can use them without
// including `magic.hh`.
inline int SpellsGetEffect(int SpellNr, int Default) {
    const TSpellList *s = SpellsGet(SpellNr);
    if (s == nullptr) return Default;
    int e = s->Effect;
    return (e != 0 ? e : Default);
}

inline int SpellsGetAnimation(int SpellNr, int Default) {
    const TSpellList *s = SpellsGet(SpellNr);
    if (s == nullptr) return Default;
    int a = s->Animation;
    return (a != 0 ? a : Default);
}

// Reload alias for callers who want a more descriptive name.  It's
// implemented as a simple wrapper around `SpellsInit`.
inline void SpellsReload(void) {
    SpellsInit();
}

#endif // TIBIA_SPELLS_HH_
