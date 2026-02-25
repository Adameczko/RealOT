#include "spells.hh"
#include "config.hh"
#include "script.hh"
#include "utils.cc"  // for FileExists, AddStaticString

#include <cstring>
#include <fstream>
#include <sstream>

// Internal storage; not visible outside this translation unit.
TSpellList SpellList[MAX_SPELLS];

// Spell syllable table used for spell word parsing and display.
const char SpellSyllable[51][6] = {
    "",
    "al",
    "ad",
    "ex",
    "ut",
    "om",
    "para",
    "ana",
    "evo",
    "ori",
    "mort",
    "lux",
    "liber",
    "vita",
    "flam",
    "pox",
    "hur",
    "moe",
    "ani",
    "ina",
    "eta",
    "amo",
    "hora",
    "gran",
    "cogni",
    "res",
    "mas",
    "vis",
    "som",
    "aqua",
    "frigo",
    "tera",
    "ura",
    "sio",
    "grav",
    "ito",
    "pan",
    "vid",
    "isa",
    "iva",
    "con",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

static int FindSpellSyllable(const char *Name){
    if(Name == NULL || Name[0] == 0){
        return -1;
    }

    for(int i = 0; i < (int)NARRAY(SpellSyllable); i += 1){
        if(strcmp(SpellSyllable[i], Name) == 0){
            return i;
        }
    }

    return -1;
}

// Forward-declare legacy loader; used as a fallback when no file is
// provided.  The implementation is copied verbatim from the old
// `magic.cc` so that the default `spells.dat` content matches exactly.
static void InitSpellsLegacy(void);

static void ApplyTemplate(int SpellNr){
    if(SpellNr <= 0 || SpellNr >= MAX_SPELLS) return;
    TSpellList &spell = SpellList[SpellNr];
    int temp = spell.Template;
    if(temp <= 0 || temp >= MAX_SPELLS) return;
    // resolve template of the template first
    if(SpellList[temp].Template != 0){
        ApplyTemplate(temp);
    }
    TSpellList &base = SpellList[temp];
    // copy every field that is "unset" (zero or NULL)
    if(spell.Mana == 0) spell.Mana = base.Mana;
    if(spell.SoulPoints == 0) spell.SoulPoints = base.SoulPoints;
    if(spell.Amount == 0) spell.Amount = base.Amount;
    if(spell.Param1 == 0) spell.Param1 = base.Param1;
    if(spell.Param2 == 0) spell.Param2 = base.Param2;
    if(spell.Param3 == 0) spell.Param3 = base.Param3;
    if(spell.Param4 == 0) spell.Param4 = base.Param4;
    if(spell.Effect == 0) spell.Effect = base.Effect;
    if(spell.Animation == 0) spell.Animation = base.Animation;
    if(spell.Flags == 0) spell.Flags = base.Flags;
    if(spell.Level == 0) spell.Level = base.Level;
    if(spell.RuneLevel == 0) spell.RuneLevel = base.RuneLevel;
    if(spell.RuneGr == 0) spell.RuneGr = base.RuneGr;
    if(spell.RuneNr == 0) spell.RuneNr = base.RuneNr;
    if(spell.Comment == NULL) spell.Comment = base.Comment;
    // syllables and words are completely replaced by the loading process
}

static void ResolveTemplates(void){
    for(int i = 1; i < MAX_SPELLS; i += 1){
        if(SpellList[i].Template != 0){
            ApplyTemplate(i);
        }
    }
}

static void LoadFromFile(const char *FileName){
    print(1, "InitSpells: Lade Zauber aus %s ...\n", FileName);

    TReadScriptFile Script;
    Script.open(FileName);

    int CurrentSpellNr = -1;
    TSpellList *CurrentSpell = NULL;

    while(true){
        Script.nextToken();
        if(Script.Token == ENDOFFILE){
            break;
        }

        char Identifier[MAX_IDENT_LENGTH];
        strcpy(Identifier, Script.getIdentifier());
        Script.readSymbol('=');

        if(strcmp(Identifier, "number") == 0){
            int SpellNr = Script.readNumber();
            if(SpellNr < 0 || SpellNr >= MAX_SPELLS){
                Script.error("illegal spell number");
            }

            CurrentSpellNr = SpellNr;
            CurrentSpell = &SpellList[SpellNr];
            memset(CurrentSpell, 0, sizeof(TSpellList));
        }else if(CurrentSpell == NULL){
            Script.error("spell property before number");
        }else if(strcmp(Identifier, "mana") == 0){
            CurrentSpell->Mana = Script.readNumber();
        }else if(strcmp(Identifier, "level") == 0){
            CurrentSpell->Level = (uint16)Script.readNumber();
        }else if(strcmp(Identifier, "flags") == 0){
            CurrentSpell->Flags = (uint16)Script.readNumber();
        }else if(strcmp(Identifier, "runegroup") == 0){
            CurrentSpell->RuneGr = (uint8)Script.readNumber();
        }else if(strcmp(Identifier, "runenumber") == 0){
            CurrentSpell->RuneNr = (uint8)Script.readNumber();
        }else if(strcmp(Identifier, "runelevel") == 0){
            CurrentSpell->RuneLevel = (uint16)Script.readNumber();
        }else if(strcmp(Identifier, "soulpoints") == 0){
            CurrentSpell->SoulPoints = Script.readNumber();
        }else if(strcmp(Identifier, "amount") == 0){
            CurrentSpell->Amount = Script.readNumber();
        }else if(strcmp(Identifier, "template") == 0
                  || strcmp(Identifier, "based_on") == 0){
            CurrentSpell->Template = Script.readNumber();
        // Generic parameter slots (spell-specific meaning, but fully data-driven).
        }else if(strcmp(Identifier, "param1") == 0){
            CurrentSpell->Param1 = Script.readNumber();
        }else if(strcmp(Identifier, "param2") == 0){
            CurrentSpell->Param2 = Script.readNumber();
        }else if(strcmp(Identifier, "param3") == 0){
            CurrentSpell->Param3 = Script.readNumber();
        }else if(strcmp(Identifier, "param4") == 0){
            CurrentSpell->Param4 = Script.readNumber();
        // Visual effect override (EFFECT_*). If zero, code defaults are used.
        }else if(strcmp(Identifier, "effect") == 0
            || strcmp(Identifier, "visual_effect") == 0
            || strcmp(Identifier, "visualeffect") == 0){
            CurrentSpell->Effect = Script.readNumber();
        }else if(strcmp(Identifier, "animation") == 0
            || strcmp(Identifier, "missile_animation") == 0
            || strcmp(Identifier, "missileanimation") == 0){
            CurrentSpell->Animation = Script.readNumber();
        // Named aliases for common parameter types, so spells.dat can stay readable.
        }else if(strcmp(Identifier, "min") == 0
                || strcmp(Identifier, "min_damage") == 0
                || strcmp(Identifier, "mindamage") == 0){
            CurrentSpell->Param1 = Script.readNumber();
        }else if(strcmp(Identifier, "delta") == 0
                || strcmp(Identifier, "delta_damage") == 0
                || strcmp(Identifier, "deltadamage") == 0){
            CurrentSpell->Param2 = Script.readNumber();
        }else if(strcmp(Identifier, "radius") == 0){
            CurrentSpell->Param3 = Script.readNumber();
        }else if(strcmp(Identifier, "angle") == 0){
            CurrentSpell->Param4 = Script.readNumber();
        }else if(strcmp(Identifier, "brightness") == 0){
            CurrentSpell->Param1 = Script.readNumber();
        }else if(strcmp(Identifier, "duration") == 0){
            CurrentSpell->Param2 = Script.readNumber();
        }else if(strcmp(Identifier, "speed_percent") == 0
                || strcmp(Identifier, "speedpercent") == 0){
            CurrentSpell->Param1 = Script.readNumber();
        }else if(strcmp(Identifier, "arrow_type") == 0
                || strcmp(Identifier, "arrowtype") == 0){
            CurrentSpell->Param1 = Script.readNumber();
        }else if(strcmp(Identifier, "arrow_count") == 0
                || strcmp(Identifier, "arrowcount") == 0){
            CurrentSpell->Param2 = Script.readNumber();
        }else if(strcmp(Identifier, "words") == 0){
            Script.readSymbol('{');

            int SyllableCount = 0;
            do{
                const char *Word = Script.readIdentifier();
                int SyllableNr = FindSpellSyllable(Word);
                if(SyllableNr <= 0){
                    Script.error("unknown spell syllable");
                }

                if(SyllableCount >= (int)NARRAY(CurrentSpell->Syllable)){
                    Script.error("too many syllables in spell");
                }

                CurrentSpell->Syllable[SyllableCount] = (uint8)SyllableNr;
                SyllableCount += 1;
            }while(Script.readSpecial() != '}');
        }else if(strcmp(Identifier, "comment") == 0){
            CurrentSpell->Comment = AddStaticString(Script.readString());
        }else{
            Script.error("unknown spell property");
        }
    }

    Script.close();
}

void SpellsInit(void){
    // reset
    for(int i = 0; i < MAX_SPELLS; i += 1){
        memset(&SpellList[i], 0, sizeof(TSpellList));
    }

    char FileName[4096];
    snprintf(FileName, sizeof(FileName), "%s/spells.dat", DATAPATH);

    if(FileExists(FileName)){
        LoadFromFile(FileName);
    } else {
        print(1, "InitSpells: Datei %s nicht gefunden, benutze eingebaute Zauber.\n", FileName);
        InitSpellsLegacy();
    }

    // after loading, apply any template-based inheritance so that the
    // rest of the engine can simply look at the fields directly.
    ResolveTemplates();
}

bool SpellsValid(int SpellNr){
    return (SpellNr > 0 && SpellNr < MAX_SPELLS && SpellList[SpellNr].Comment != NULL);
}

const TSpellList *SpellsGet(int SpellNr){
    if(SpellNr <= 0 || SpellNr >= MAX_SPELLS) return nullptr;
    return &SpellList[SpellNr];
}

// Legacy loader copied from magic.cc, kept for fallback compatibility.
static TSpellList *CreateSpell(int SpellNr, ...){
    ASSERT(SpellNr < MAX_SPELLS);
    int SyllableCount = 0;
    TSpellList *Spell = &SpellList[SpellNr];

    va_list ap;
    va_start(ap, SpellNr);
    while(true){
        const char *Syllable = va_arg(ap, const char*);
        if(!Syllable || Syllable[0] == 0){
            break;
        }

        for(int SyllableNr = 0;
                SyllableNr < NARRAY(SpellSyllable);
                SyllableNr += 1){
            if(strcmp(SpellSyllable[SyllableNr], Syllable) == 0){
                if(SyllableCount > NARRAY(Spell->Syllable)){
                    error("CreateSpell: Silbenzahl überschritten bei Spell %d.\n", SpellNr);
                    throw "Spell has too many syllables";
                }

                Spell->Syllable[SyllableCount] = (uint8)SyllableNr;
                SyllableCount += 1;
                break;
            }
        }
    }
    va_end(ap);
    return Spell;
}

static void InitSpellsLegacy(void){
    TSpellList *Spell;

    Spell = CreateSpell(1, "ex", "ura", "");
    Spell->Mana = 25;
    Spell->Level = 9;
    Spell->Flags = 8;
    Spell->Comment = "Light Healing";

    Spell = CreateSpell(2, "ex", "ura", "gran", "");
    Spell->Mana = 40;
    Spell->Level = 11;
    Spell->Flags = 8;
    Spell->Comment = "Intense Healing";

    Spell = CreateSpell(3, "ex", "ura", "vita", "");
    Spell->Mana = 160;
    Spell->Level = 20;
    Spell->Flags = 8;
    Spell->Comment = "Ultimate Healing";

    Spell = CreateSpell(4, "ad", "ura", "gran", "");
    Spell->Mana = 240;
    Spell->Level = 15;
    Spell->RuneGr = 79;
    Spell->RuneNr = 5;
    Spell->Flags = 8;
    Spell->Amount = 1;
    Spell->RuneLevel = 1;
    Spell->SoulPoints = 2;
    Spell->Comment = "Intense Healing Rune";

    Spell = CreateSpell(5, "ad", "ura", "vita", "");
    Spell->Mana = 400;
    Spell->Level = 24;
    Spell->RuneGr = 79;
    Spell->RuneNr = 13;
    Spell->Flags = 8;
    Spell->Amount = 1;
    Spell->RuneLevel = 4;
    Spell->SoulPoints = 3;
    Spell->Comment = "Ultimate Healing Rune";

    Spell = CreateSpell(6, "ut", "ani", "hur", "");
    Spell->Mana = 60;
    Spell->Level = 14;
    Spell->Flags = 2;
    Spell->Comment = "Haste";

    Spell = CreateSpell(7, "ad", "ori", "");
    Spell->Mana = 120;
    Spell->Level = 15;
    Spell->RuneGr = 79;
    Spell->RuneNr = 27;
    Spell->Flags = 9;
    Spell->Amount = 5;
    Spell->RuneLevel = 0;
    Spell->SoulPoints = 1;
    Spell->Comment = "Light Magic Missile";

    Spell = CreateSpell(8, "ad", "ori", "gran", "");
    Spell->Mana = 280;
    Spell->Level = 25;
    Spell->RuneGr = 79;
    Spell->RuneNr = 51;
    Spell->Flags = 9;
    Spell->Amount = 5;
    Spell->RuneLevel = 4;
    Spell->SoulPoints = 2;
    Spell->Comment = "Heavy Magic Missile";

    Spell = CreateSpell(9, "ut", "evo", "res", "para", "");
    Spell->Mana = 0;
    Spell->Level = 25;
    Spell->Flags = 1;
    Spell->Comment = "Summon Creature";

    Spell = CreateSpell(10, "ut", "evo", "lux", "");
    Spell->Mana = 20;
    Spell->Level = 8;
    Spell->Flags = 0;
    Spell->Comment = "Light";

    Spell = CreateSpell(11, "ut", "evo", "gran", "lux", "");
    Spell->Mana = 60;
    Spell->Level = 13;
    Spell->Flags = 0;
    Spell->Comment = "Great Light";

    Spell = CreateSpell(12, "ad", "eta", "sio", "");
    Spell->Mana = 200;
    Spell->Level = 16;
    Spell->RuneGr = 79;
    Spell->RuneNr = 30;
    Spell->Flags = 1;
    Spell->Amount = 1;
    Spell->RuneLevel = 5;
    Spell->SoulPoints = 3;
    Spell->Comment = "Convince Creature";

    Spell = CreateSpell(13, "ex", "evo", "mort", "hur", "");
    Spell->Mana = 250;
    Spell->Level = 38;
    Spell->Flags = 1;
    Spell->Comment = "Energy Wave";

    Spell = CreateSpell(14, "ad", "evo", "ina", "");
    Spell->Mana = 600;
    Spell->Level = 27;
    Spell->RuneGr = 79;
    Spell->RuneNr = 31;
    Spell->Flags = 0;
    Spell->Amount = 1;
    Spell->RuneLevel = 4;
    Spell->SoulPoints = 2;
    Spell->Comment = "Chameleon";

    Spell = CreateSpell(15, "ad", "ori", "flam", "");
    Spell->Mana = 160;
    Spell->Level = 17;
    Spell->RuneGr = 79;
    Spell->RuneNr = 42;
    Spell->Flags = 9;
    Spell->Amount = 2;
    Spell->RuneLevel = 2;
    Spell->SoulPoints = 2;
    Spell->Comment = "Fireball";

    Spell = CreateSpell(16, "ad", "ori", "gran", "flam", "");
    Spell->Mana = 480;
    Spell->Level = 23;
    Spell->RuneGr = 79;
    Spell->RuneNr = 44;
    Spell->Flags = 9;
    Spell->Amount = 2;
    Spell->RuneLevel = 4;
    Spell->SoulPoints = 3;
    Spell->Comment = "Great Fireball";

    Spell = CreateSpell(17, "ad", "evo", "mas", "flam", "");
    Spell->Mana = 600;
    Spell->Level = 27;
    Spell->RuneGr = 79;
    Spell->RuneNr = 45;
    Spell->Flags = 1;
    Spell->Amount = 2;
    Spell->RuneLevel = 5;
    Spell->SoulPoints = 4;
    Spell->Comment = "Firebomb";

    Spell = CreateSpell(18, "ad", "evo", "mas", "hur", "");
    Spell->Mana = 720;
    Spell->Level = 31;
    Spell->RuneGr = 79;
    Spell->RuneNr = 53;
    Spell->Flags = 1;
    Spell->Amount = 3;
    Spell->RuneLevel = 6;
    Spell->SoulPoints = 4;
    Spell->Comment = "Explosion";

    Spell = CreateSpell(19, "ex", "evo", "flam", "hur", "");
    Spell->Mana = 80;
    Spell->Level = 18;
    Spell->Flags = 9;
    Spell->Comment = "Fire Wave";

    Spell = CreateSpell(20, "ex", "iva", "para", "");
    Spell->Mana = 20;
    Spell->Level = 8;
    Spell->Flags = 0;
    Spell->Comment = "Find Person";

    Spell = CreateSpell(21, "ad", "ori", "vita", "vis", "");
    Spell->Mana = 880;
    Spell->Level = 45;
    Spell->RuneGr = 79;
    Spell->RuneNr = 8;
    Spell->Flags = 1;
    Spell->Amount = 1;
    Spell->RuneLevel = 15;
    Spell->SoulPoints = 5;
    Spell->Comment = "Sudden Death";

    Spell = CreateSpell(22, "ex", "evo", "vis", "lux", "");
    Spell->Mana = 100;
    Spell->Level = 23;
    Spell->Flags = 1;
    Spell->Comment = "Energy Beam";

    Spell = CreateSpell(23, "ex", "evo", "gran", "vis", "lux", "");
    Spell->Mana = 200;
    Spell->Level = 29;
    Spell->Flags = 1;
    Spell->Comment = "Great Energy Beam";

    Spell = CreateSpell(24, "ex", "evo", "gran", "mas", "vis", "");
    Spell->Mana = 1200;
    Spell->Level = 60;
    Spell->Flags = 3;
    Spell->Comment = "Ultimate Explosion";

    Spell = CreateSpell(25, "ad", "evo", "grav", "flam", "");
    Spell->Mana = 240;
    Spell->Level = 15;
    Spell->RuneGr = 79;
    Spell->RuneNr = 41;
    Spell->Flags = 1;
    Spell->Amount = 3;
    Spell->RuneLevel = 1;
    Spell->SoulPoints = 1;
    Spell->Comment = "Fire Field";

    Spell = CreateSpell(26, "ad", "evo", "grav", "pox", "");
    Spell->Mana = 200;
    Spell->Level = 14;
    Spell->RuneGr = 79;
    Spell->RuneNr = 25;
    Spell->Flags = 1;
    Spell->Amount = 3;
    Spell->RuneLevel = 0;
    Spell->SoulPoints = 1;
    Spell->Comment = "Poison Field";

    Spell = CreateSpell(27, "ad", "evo", "grav", "vis", "");
    Spell->Mana = 320;
    Spell->Level = 18;
    Spell->RuneGr = 79;
    Spell->RuneNr = 17;
    Spell->Flags = 1;
    Spell->Amount = 3;
    Spell->RuneLevel = 3;
    Spell->SoulPoints = 2;
    Spell->Comment = "Energy Field";

    Spell = CreateSpell(28, "ad", "evo", "mas", "grav", "flam", "");
    Spell->Mana = 780;
    Spell->Level = 33;
    Spell->RuneGr = 79;
    Spell->RuneNr = 43;
    Spell->Flags = 1;
    Spell->Amount = 4;
    Spell->RuneLevel = 6;
    Spell->SoulPoints = 4;
    Spell->Comment = "Fire Wall";

    Spell = CreateSpell(29, "ex", "ana", "pox", "");
    Spell->Mana = 30;
    Spell->Level = 10;
    Spell->Flags = 0;
    Spell->Comment = "Antidote";

    Spell = CreateSpell(30, "ad", "ito", "grav", "");
    Spell->Mana = 120;
    Spell->Level = 17;
    Spell->RuneGr = 79;
    Spell->RuneNr = 1;
    Spell->Flags = 0;
    Spell->Amount = 3;
    Spell->RuneLevel = 3;
    Spell->SoulPoints = 2;
    Spell->Comment = "Destroy Field";

    Spell = CreateSpell(31, "ad", "ana", "pox", "");
    Spell->Mana = 200;
    Spell->Level = 15;
    Spell->RuneGr = 79;
    Spell->RuneNr = 6;
    Spell->Flags = 0;
    Spell->Amount = 1;
    Spell->RuneLevel = 0;
    Spell->SoulPoints = 1;
    Spell->Comment = "Antidote Rune";

    Spell = CreateSpell(32, "ad", "evo", "mas", "grav", "pox", "");
    Spell->Mana = 640;
    Spell->Level = 29;
    Spell->RuneGr = 79;
    Spell->RuneNr = 29;
    Spell->Flags = 1;
    Spell->Amount = 4;
    Spell->RuneLevel = 5;
    Spell->SoulPoints = 3;
    Spell->Comment = "Poison Wall";

    Spell = CreateSpell(33, "ad", "evo", "mas", "grav", "vis", "");
    Spell->Mana = 1000;
    Spell->Level = 41;
    Spell->RuneGr = 79;
    Spell->RuneNr = 19;
    Spell->Flags = 1;
    Spell->Amount = 4;
    Spell->RuneLevel = 9;
    Spell->SoulPoints = 5;
    Spell->Comment = "Energy Wall";

    Spell = CreateSpell(34, "al", "evo", "para", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Get Item";

    Spell = CreateSpell(35, "al", "evo", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Get Item";

    Spell = CreateSpell(37, "al", "ani", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Move";

    Spell = CreateSpell(38, "ut", "evo", "res", "ina", "para", "");
    Spell->Mana = 100;
    Spell->Level = 23;
    Spell->Flags = 0;
    Spell->Comment = "Creature Illusion";

    Spell = CreateSpell(39, "ut", "ani", "gran", "hur", "");
    Spell->Mana = 100;
    Spell->Level = 20;
    Spell->Flags = 2;
    Spell->Comment = "Strong Haste";

    Spell = CreateSpell(40, "al", "evo", "cogni", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Get Experience";

    Spell = CreateSpell(41, "al", "eta", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Change Data";

    Spell = CreateSpell(42, "ex", "evo", "pan", "");
    Spell->Mana = 120;
    Spell->Level = 14;
    Spell->SoulPoints = 1;
    Spell->Flags = 0;
    Spell->Comment = "Food";

    Spell = CreateSpell(44, "ut", "amo", "vita", "");
    Spell->Mana = 50;
    Spell->Level = 14;
    Spell->Flags = 0;
    Spell->Comment = "Magic Shield";

    Spell = CreateSpell(45, "ut", "ana", "vid", "");
    Spell->Mana = 440;
    Spell->Level = 35;
    Spell->Flags = 0;
    Spell->Comment = "Invisible";

    Spell = CreateSpell(46, "al", "evo", "cogni", "para", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Get Skill Experience";

    Spell = CreateSpell(47, "al", "ani", "sio", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Teleport to Friend";

    Spell = CreateSpell(48, "ex", "evo", "con", "pox", "");
    Spell->Mana = 130;
    Spell->Level = 16;
    Spell->SoulPoints = 2;
    Spell->Flags = 0;
    Spell->Comment = "Poisoned Arrow";

    Spell = CreateSpell(49, "ex", "evo", "con", "flam", "");
    Spell->Mana = 290;
    Spell->Level = 25;
    Spell->SoulPoints = 3;
    Spell->Flags = 0;
    Spell->Comment = "Explosive Arrow";

    Spell = CreateSpell(50, "ad", "evo", "res", "flam", "");
    Spell->Mana = 600;
    Spell->Level = 27;
    Spell->RuneGr = 79;
    Spell->RuneNr = 48;
    Spell->Flags = 3;
    Spell->Amount = 2;
    Spell->RuneLevel = 7;
    Spell->SoulPoints = 3;
    Spell->Comment = "Soulfire";

    Spell = CreateSpell(51, "ex", "evo", "con", "");
    Spell->Mana = 100;
    Spell->Level = 13;
    Spell->SoulPoints = 1;
    Spell->Flags = 0;
    Spell->Comment = "Conjure Arrow";

    Spell = CreateSpell(52, "al", "liber", "sio", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Retrieve Friend";

    Spell = CreateSpell(53, "al", "evo", "res", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Summon Wild Creature";

    Spell = CreateSpell(54, "ad", "ana", "ani", "");
    Spell->Mana = 1400;
    Spell->Level = 54;
    Spell->RuneGr = 79;
    Spell->RuneNr = 18;
    Spell->Flags = 3;
    Spell->Amount = 1;
    Spell->RuneLevel = 18;
    Spell->SoulPoints = 3;
    Spell->Comment = "Paralyze";

    Spell = CreateSpell(55, "ad", "evo", "mas", "vis", "");
    Spell->Mana = 880;
    Spell->Level = 37;
    Spell->RuneGr = 79;
    Spell->RuneNr = 2;
    Spell->Flags = 3;
    Spell->Amount = 2;
    Spell->RuneLevel = 10;
    Spell->SoulPoints = 5;
    Spell->Comment = "Energybomb";

    Spell = CreateSpell(56, "ex", "evo", "gran", "mas", "pox", "");
    Spell->Mana = 600;
    Spell->Level = 50;
    Spell->Flags = 3;
    Spell->Comment = "Poison Storm";

    Spell = CreateSpell(57, "om", "ana", "liber", "para", "para", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Banish Account";

    Spell = CreateSpell(58, "al", "iva", "tera", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Get Position";

    Spell = CreateSpell(60, "om", "ani", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Temple Teleport";

    Spell = CreateSpell(61, "om", "ana", "gran", "liber", "para", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Delete Account";

    Spell = CreateSpell(62, "om", "amo", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Set Namerule";

    Spell = CreateSpell(63, "al", "evo", "vis", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Create Gold";

    Spell = CreateSpell(64, "al", "eta", "vita", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Change Profession or Sex";

    Spell = CreateSpell(65, "om", "isa", "para", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Entry in Criminal Record";

    Spell = CreateSpell(66, "om", "ana", "hora", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Namelock";

    Spell = CreateSpell(67, "om", "ana", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Kick Player";

    Spell = CreateSpell(68, "om", "ana", "gran", "res", "para", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Delete Character";

    Spell = CreateSpell(69, "om", "ana", "vis", "para", "para", "");
    Spell->Mana = 0;
    Spell->Level = 0;
    Spell->Flags = 0;
    Spell->Comment = "Banish IP Address";

    Spell = CreateSpell(70, "om", "ana", "res", "para", "para", "para", "");
    Spell->Mana = 0;
    Spell->Strength? (Truncated due to length)
