#include "randomizer_entrance_tracker.h"
#include "soh/OTRGlobals.h"
#include <ImGuiImpl.h>
#include "soh/UIWidgets.hpp"

#include <map>
#include <string>
#include <vector>
#include <libultraship/bridge.h>
#include <Hooks.h>

extern "C" {
#include <z64.h>
#include "variables.h"
#include "functions.h"
#include "macros.h"
extern PlayState* gPlayState;

#include "soh/Enhancements/randomizer/randomizer_entrance.h"
#include "soh/Enhancements/randomizer/randomizer_grotto.h"
}

#include "soh/Enhancements/game-interactor/GameInteractor.h"

#define COLOR_ORANGE IM_COL32(230, 159, 0, 255)
#define COLOR_GREEN IM_COL32(0, 158, 115, 255)
#define COLOR_GRAY IM_COL32(155, 155, 155, 255)

EntranceOverride srcListSortedByArea[ENTRANCE_OVERRIDES_MAX_COUNT] = {0};
EntranceOverride destListSortedByArea[ENTRANCE_OVERRIDES_MAX_COUNT] = {0};
EntranceOverride srcListSortedByType[ENTRANCE_OVERRIDES_MAX_COUNT] = {0};
EntranceOverride destListSortedByType[ENTRANCE_OVERRIDES_MAX_COUNT] = {0};
EntranceTrackingData gEntranceTrackingData = {0};

static const EntranceOverride emptyOverride = {0};

static s16 lastEntranceIndex = -1;
static s16 currentGrottoId = -1;
static s16 lastSceneOrEntranceDetected = -1;

static std::string spoilerEntranceGroupNames[] = {
    "Inicios/Canciones/Búhos",
    "Bosque Kokiri",
    "Bosque Perdido",
    "Pradera Sagrada",
    "Kakariko",
    "Cementerio",
    "Sendero de la Montaña de la Muerte",
    "Cráter de la Montaña de la Muerte",
    "Ciudad Goron",
    "Río Zora",
    "Región de los zora",
    "Fuente Zora",
    "Llanura de Hyrule",
    "Rancho Lon Lon",
    "Lago Hylia",
    "Valle Gerudo",
    "Desierto encantado",
    "Plaza del mercado",
    "Castillo de Hyrule",
};

static std::string groupTypeNames[] = {
    "Sentido único",
    "Exterior",
    "Interior",
    "Gruta",
    "Mazmorra",
};

// Entrance data for the tracker taken from the 3ds rando entrance tracker, and supplemented with scene/spawn info and meta search tags
const EntranceData entranceData[] = {
    //index,  reverse, scenes (and spawns),     source name,   destination name, source group,           destination group,      type,                 metaTag, oneExit
    { 0x00BB, -1,      SINGLE_SCENE_INFO(0x34), "Inicio de niño", "Casa de Link",   ENTRANCE_GROUP_ONE_WAY, ENTRANCE_GROUP_ONE_WAY, ENTRANCE_TYPE_ONE_WAY},
    { 0x0282, -1,      SINGLE_SCENE_INFO(0x43), "Inicio de adulto", "Templo del Tiempo", ENTRANCE_GROUP_ONE_WAY, ENTRANCE_GROUP_ONE_WAY, ENTRANCE_TYPE_ONE_WAY},

    { 0x0600, -1, {{ -1 }}, "Minueto del bosque",   "Teletransporte de la Pradera",              ENTRANCE_GROUP_ONE_WAY, ENTRANCE_GROUP_ONE_WAY, ENTRANCE_TYPE_ONE_WAY},
    { 0x04F6, -1, {{ -1 }}, "Bolero del fuego",     "Teletransporte del Cráter",              ENTRANCE_GROUP_ONE_WAY, ENTRANCE_GROUP_ONE_WAY, ENTRANCE_TYPE_ONE_WAY},
    { 0x0604, -1, {{ -1 }}, "Serenata del agua",  "Teletransporte del Lago",       ENTRANCE_GROUP_ONE_WAY, ENTRANCE_GROUP_ONE_WAY, ENTRANCE_TYPE_ONE_WAY},
    { 0x01F1, -1, {{ -1 }}, "Réquiem del espíritu",  "Teletransporte del Coloso",  ENTRANCE_GROUP_ONE_WAY, ENTRANCE_GROUP_ONE_WAY, ENTRANCE_TYPE_ONE_WAY},
    { 0x0568, -1, {{ -1 }}, "Nocturno de la sombra", "Teletransporte del Cementerio",        ENTRANCE_GROUP_ONE_WAY, ENTRANCE_GROUP_ONE_WAY, ENTRANCE_TYPE_ONE_WAY},
    { 0x05F4, -1, {{ -1 }}, "Preludio de la luz",   "Teletransporte del Templo del Tiempo",   ENTRANCE_GROUP_ONE_WAY, ENTRANCE_GROUP_ONE_WAY, ENTRANCE_TYPE_ONE_WAY},

    { 0x0554, -1, SINGLE_SCENE_INFO(0x60), "Vuelo del búho (Sendero)", "Aterrizaje del búho (Kakariko)", ENTRANCE_GROUP_ONE_WAY, ENTRANCE_GROUP_ONE_WAY, ENTRANCE_TYPE_ONE_WAY},
    { 0x027E, -1, SINGLE_SCENE_INFO(0x57), "Vuelo del búho (Lago)",  "Aterrizaje del búho (Llanura)",     ENTRANCE_GROUP_ONE_WAY, ENTRANCE_GROUP_ONE_WAY, ENTRANCE_TYPE_ONE_WAY},

    // Kokiri Forest
    { 0x05E0, 0x020D, SINGLE_SCENE_INFO(0x55), "Bosque Kokiri",                  "Puente del Bosque Perdido",   ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_LOST_WOODS,    ENTRANCE_TYPE_OVERWORLD, "lw"},
    { 0x011E, 0x0286, SINGLE_SCENE_INFO(0x55), "Bosque Kokiri",                  "Bosque Perdido",          ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_LOST_WOODS,    ENTRANCE_TYPE_OVERWORLD, "lw"},
    { 0x0272, 0x0211, SINGLE_SCENE_INFO(0x55), "Bosque Kokiri",                  "Casa de Link",        ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x0433, 0x0443, SINGLE_SCENE_INFO(0x55), "Bosque Kokiri",                  "Casa de Mido",        ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x0437, 0x0447, SINGLE_SCENE_INFO(0x55), "Bosque Kokiri",                  "Casa de Saria",       ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x009C, 0x033C, SINGLE_SCENE_INFO(0x55), "Bosque Kokiri",                  "Casa de los gemelos",      ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x00C9, 0x026A, SINGLE_SCENE_INFO(0x55), "Bosque Kokiri",                  "Casa de los Sabelotodo",   ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x00C1, 0x0266, SINGLE_SCENE_INFO(0x55), "Bosque Kokiri",                  "Tienda Kokiri",             ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x071B, 0x081B, SINGLE_SCENE_INFO(0x55), "Bosque Kokiri",                  "Gruta de la tormenta (Bosque Kokiri)",    ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_GROTTO,    "chest", 1},
    { 0x0000, 0x0209, SINGLE_SCENE_INFO(0x55), "Bosque Kokiri",                  "Gran Árbol Deku",           ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_DUNGEON,   "", 1},
    { 0x0211, 0x0272, SINGLE_SCENE_INFO(0x34), "Casa de Link",        "Bosque Kokiri",                  ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_INTERIOR,  ""},
    { 0x0443, 0x0433, SINGLE_SCENE_INFO(0x28), "Casa de Mido",        "Bosque Kokiri",                  ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_INTERIOR,  ""},
    { 0x0447, 0x0437, SINGLE_SCENE_INFO(0x29), "Casa de Saria",       "Bosque Kokiri",                  ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_INTERIOR,  ""},
    { 0x033C, 0x009C, SINGLE_SCENE_INFO(0x27), "Casa de los gemelos",      "Bosque Kokiri",                  ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_INTERIOR,  ""},
    { 0x026A, 0x00C9, SINGLE_SCENE_INFO(0x26), "Casa de los Sabelotodo",   "Bosque Kokiri",                  ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_INTERIOR,  ""},
    { 0x0266, 0x00C1, SINGLE_SCENE_INFO(0x2D), "Tienda Kokiri",             "Bosque Kokiri",                  ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_INTERIOR,  ""},
    { 0x081B, 0x071B, {{ 0x3E, 0x00 }},        "Gruta de la tormenta (Bosque Kokiri)",    "Bosque Kokiri",                  ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_GROTTO,    "chest"},
    { 0x0209, 0x0000, SINGLE_SCENE_INFO(0x00), "Gran Árbol Deku",           "Bosque Kokiri",                  ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_DUNGEON,   ""},
    { 0x040F, 0x0252, SINGLE_SCENE_INFO(0x00), "Puerta del jefe del Árbol Deku", "Gohma",               ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_DUNGEON,   "", 1},
    { 0x0252, 0x040F, SINGLE_SCENE_INFO(0x11), "Gohma",               "Puerta del jefe del Árbol Deku", ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_DUNGEON,   "", 1},

    // Lost Woods
    { 0x020D, 0x05E0, SINGLE_SCENE_INFO(0x5B), "Puente del Bosque Perdido",        "Bosque Kokiri",                       ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_OVERWORLD, "lw"},
    { 0x0185, 0x04DE, SINGLE_SCENE_INFO(0x5B), "Puente del Bosque Perdido",        "Llanura de Hyrule",             ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_OVERWORLD, "lw,hf"},
    { 0x0286, 0x011E, SINGLE_SCENE_INFO(0x5B), "Bosque Perdido",               "Bosque Kokiri",                       ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_GROUP_KOKIRI_FOREST, ENTRANCE_TYPE_OVERWORLD, "lw"},
    { 0x04E2, 0x04D6, SINGLE_SCENE_INFO(0x5B), "Bosque Perdido",               "Ciudad Goron",               ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_GROUP_GORON_CITY,    ENTRANCE_TYPE_OVERWORLD, "lw,gc"},
    { 0x01DD, 0x04DA, SINGLE_SCENE_INFO(0x5B), "Bosque Perdido",               "Río Zora",                       ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_GROUP_ZORAS_RIVER,   ENTRANCE_TYPE_OVERWORLD, "lw"},
    { 0x00FC, 0x01A9, SINGLE_SCENE_INFO(0x5B), "Bosque Perdido",               "Pradera Sagrada",                      ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_GROUP_SFM,           ENTRANCE_TYPE_OVERWORLD, "lw"},
    { 0x071A, 0x081A, SINGLE_SCENE_INFO(0x5B), "Bosque Perdido",               "Gruta de atajos (Bosque Perdido)", ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_GROUP_LOST_WOODS,    ENTRANCE_TYPE_GROTTO,    "lw,chest", 1},
    { 0x0719, 0x0819, SINGLE_SCENE_INFO(0x5B), "Bosque Perdido",               "Gruta de los deku (Bosque Perdido)",         ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_GROUP_LOST_WOODS,    ENTRANCE_TYPE_GROTTO,    "lw", 1},
    { 0x0720, 0x0820, SINGLE_SCENE_INFO(0x5B), "Bosque Perdido",               "Teatro Deku",             ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_GROUP_LOST_WOODS,    ENTRANCE_TYPE_GROTTO,    "lw,mask,stage", 1},
    { 0x081A, 0x071A, {{ 0x3E, 0x00 }},        "Gruta de atajos (Bosque Perdido)", "Bosque Perdido",               ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_GROUP_LOST_WOODS,    ENTRANCE_TYPE_GROTTO,    "lw,chest"},
    { 0x0819, 0x0719, {{ 0x3E, 0x07 }},        "Gruta de los deku (Bosque Perdido)",         "Bosque Perdido",               ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_GROUP_LOST_WOODS,    ENTRANCE_TYPE_GROTTO,    "lw"},
    { 0x0820, 0x0720, {{ 0x3E, 0x0C }},        "Teatro Deku",             "Bosque Perdido",               ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_GROUP_LOST_WOODS,    ENTRANCE_TYPE_GROTTO,    "lw,mask,stage"},

    // Sacred Forest Meadow
    { 0x01A9, 0x00FC, SINGLE_SCENE_INFO(0x56), "Pradera Sagrada",                     "Bosque Perdido",              ENTRANCE_GROUP_SFM, ENTRANCE_GROUP_LOST_WOODS, ENTRANCE_TYPE_OVERWORLD, "lw"},
    { 0x0716, 0x0816, SINGLE_SCENE_INFO(0x56), "Pradera Sagrada",                     "Gruta de lobos (Pradera)",       ENTRANCE_GROUP_SFM, ENTRANCE_GROUP_SFM,        ENTRANCE_TYPE_GROTTO,    "chest", 1},
    { 0x0718, 0x0818, SINGLE_SCENE_INFO(0x56), "Pradera Sagrada",                     "Gruta de las hadas (Pradera)",        ENTRANCE_GROUP_SFM, ENTRANCE_GROUP_SFM,        ENTRANCE_TYPE_GROTTO,    "", 1},
    { 0x0717, 0x0817, SINGLE_SCENE_INFO(0x56), "Pradera Sagrada",                     "Gruta de la tormenta (Pradera)",       ENTRANCE_GROUP_SFM, ENTRANCE_GROUP_SFM,        ENTRANCE_TYPE_GROTTO,    "scrubs", 1},
    { 0x0169, 0x0215, SINGLE_SCENE_INFO(0x56), "Pradera Sagrada",                     "Templo del Bosque",           ENTRANCE_GROUP_SFM, ENTRANCE_GROUP_SFM,        ENTRANCE_TYPE_DUNGEON,   "", 1},
    { 0x0816, 0x0716, {{ 0x3E, 0x08 }},        "Gruta de lobos (Pradera)",       "Pradera Sagrada",                     ENTRANCE_GROUP_SFM, ENTRANCE_GROUP_SFM,        ENTRANCE_TYPE_GROTTO},
    { 0x0818, 0x0718, {{ 0x3C, 0x00 }},        "Gruta de las hadas (Pradera)",        "Pradera Sagrada",                     ENTRANCE_GROUP_SFM, ENTRANCE_GROUP_SFM,        ENTRANCE_TYPE_GROTTO},
    { 0x0817, 0x0717, {{ 0x3E, 0x0A }},        "Gruta de la tormenta (Pradera)",       "Pradera Sagrada",                     ENTRANCE_GROUP_SFM, ENTRANCE_GROUP_SFM,        ENTRANCE_TYPE_GROTTO,    "scrubs"},
    { 0x0215, 0x0169, SINGLE_SCENE_INFO(0x03), "Templo del Bosque",           "Pradera Sagrada",                     ENTRANCE_GROUP_SFM, ENTRANCE_GROUP_SFM,        ENTRANCE_TYPE_DUNGEON},
    { 0x000C, 0x024E, SINGLE_SCENE_INFO(0x03), "Puerta del jefe del Templo del Bosque", "Ganon fantasma",           ENTRANCE_GROUP_SFM, ENTRANCE_GROUP_SFM,        ENTRANCE_TYPE_DUNGEON, "", 1},
    { 0x024E, 0x000C, SINGLE_SCENE_INFO(0x14), "Ganon fantasma",           "Puerta del jefe del Templo del Bosque", ENTRANCE_GROUP_SFM, ENTRANCE_GROUP_SFM,        ENTRANCE_TYPE_DUNGEON, "", 1},

    // Kakariko Village
    { 0x017D, 0x00DB, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Llanura de Hyrule",          ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_HYRULE_FIELD,         ENTRANCE_TYPE_OVERWORLD, "hf"},
    { 0x00E4, 0x0195, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Cementerio",             ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_GRAVEYARD,            ENTRANCE_TYPE_OVERWORLD},
    { 0x013D, 0x0191, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Sendero",                   ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_TYPE_OVERWORLD},
    { 0x02FD, 0x0349, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Casa del capataz",  ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x0550, 0x04EE, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Casa de las Skulltulas",    ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x039C, 0x0345, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Casa de Impa (delante)",    ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x05C8, 0x05DC, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Casa de Impa (atrás)",     ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR,  "cow", 1},
    { 0x0453, 0x0351, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Molino",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x003B, 0x0463, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Galería de tiro de Kakariko",  ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR,  "adult", 1},
    { 0x0072, 0x034D, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Tienda de pociones de la abuela",  ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x00B7, 0x0201, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Bazar de Kakariko",            ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR,  "shop", 1},
    { 0x0384, 0x044B, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Tienda de pociones de Kakariko (delante)", ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x03EC, 0x04FF, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Tienda de pociones de Kakariko (atrás)",  ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x070A, 0x080A, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Gruta abierta (Kakariko)",       ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_GROTTO,    "chest", 1},
    { 0x070B, 0x080B, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Gruta Redead (Kakariko)",     ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_GROTTO,    "chest", 1},
    { 0x0098, 0x02A6, SINGLE_SCENE_INFO(0x52), "Kakariko",              "Pozo de Kakariko",    ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_DUNGEON,   "botw", 1},
    { 0x0349, 0x02FD, SINGLE_SCENE_INFO(0x2A), "Casa del capataz",  "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR},
    { 0x04EE, 0x0550, SINGLE_SCENE_INFO(0x50), "Casa de las Skulltulas",    "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR},
    { 0x0345, 0x039C, SINGLE_SCENE_INFO(0x37), "Casa de Impa (delante)",    "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR},
    { 0x05DC, 0x05C8, SINGLE_SCENE_INFO(0x37), "Casa de Impa (atrás)",     "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR,  "cow"},
    { 0x0351, 0x0453, SINGLE_SCENE_INFO(0x48), "Molino",              "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR},
    { 0x0463, 0x003B, {{ 0x42, 0x00 }},        "Galería de tiro de Kakariko",  "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR},
    { 0x034D, 0x0072, SINGLE_SCENE_INFO(0x4E), "Tienda de pociones de la abuela",  "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR},
    { 0x0201, 0x00B7, {{ 0x2C, 0x00 }},        "Bazar de Kakariko",            "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR,  "shop"},
    { 0x044B, 0x0384, SINGLE_SCENE_INFO(0x30), "Tienda de pociones de Kakariko (delante)", "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR},
    { 0x04FF, 0x03EC, SINGLE_SCENE_INFO(0x30), "Tienda de pociones de Kakariko (atrás)",  "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_INTERIOR},
    { 0x080A, 0x070A, {{ 0x3E, 0x00 }},        "Gruta abierta (Kakariko)",       "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_GROTTO,    "chest"},
    { 0x080B, 0x070B, {{ 0x3E, 0x03 }},        "Gruta Redead (Kakariko)",     "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_GROTTO,    "chest"},
    { 0x02A6, 0x0098, SINGLE_SCENE_INFO(0x08), "Pozo de Kakariko",    "Kakariko",              ENTRANCE_GROUP_KAKARIKO, ENTRANCE_GROUP_KAKARIKO,             ENTRANCE_TYPE_DUNGEON,   "botw"},

    // The Graveyard
    { 0x0195, 0x00E4, SINGLE_SCENE_INFO(0x53), "Cementerio",               "Kakariko",                ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_KAKARIKO,  ENTRANCE_TYPE_OVERWORLD},
    { 0x030D, 0x0355, SINGLE_SCENE_INFO(0x53), "Cementerio",               "Cabaña de Dampé",           ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_INTERIOR, "", 1},
    { 0x004B, 0x035D, SINGLE_SCENE_INFO(0x53), "Cementerio",               "Tumba del escudo",            ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_GROTTO,   "", 1},
    { 0x031C, 0x0361, SINGLE_SCENE_INFO(0x53), "Cementerio",               "Tumba de la pieza de corazón",       ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_GROTTO,   "", 1},
    { 0x002D, 0x050B, SINGLE_SCENE_INFO(0x53), "Cementerio",               "Tumba de los compositores",        ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_GROTTO,   "", 1},
    { 0x044F, 0x0359, SINGLE_SCENE_INFO(0x53), "Cementerio",               "Tumba de Dampé",           ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_GROTTO,   "race", 1},
    { 0x0037, 0x0205, SINGLE_SCENE_INFO(0x53), "Cementerio",               "Templo de las Sombras",           ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_DUNGEON,  "", 1},
    { 0x0355, 0x030D, SINGLE_SCENE_INFO(0x3A), "Cabaña de Dampé",           "Cementerio",               ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_INTERIOR},
    { 0x035D, 0x004B, SINGLE_SCENE_INFO(0x40), "Tumba del escudo",            "Cementerio",               ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_GROTTO},
    { 0x0361, 0x031C, SINGLE_SCENE_INFO(0x3F), "Tumba de la pieza de corazón",       "Cementerio",               ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_GROTTO},
    { 0x050B, 0x002D, SINGLE_SCENE_INFO(0x41), "Tumba de los compositores",        "Cementerio",               ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_GROTTO},
    { 0x0359, 0x044F, SINGLE_SCENE_INFO(0x48), "Tumba de Dampé",           "Cementerio",               ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_GROTTO,   "race"},
    { 0x0205, 0x0037, SINGLE_SCENE_INFO(0x07), "Templo de las Sombras",           "Cementerio",               ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_DUNGEON},
    { 0x0413, 0x02B2, SINGLE_SCENE_INFO(0x07), "Puerta del jefe del Templo de las Sombras", "Bongo Bongo",             ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_DUNGEON, "", 1},
    { 0x02B2, 0x0413, SINGLE_SCENE_INFO(0x18), "Bongo Bongo",             "Puerta del jefe del Templo de las Sombras", ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_GROUP_GRAVEYARD, ENTRANCE_TYPE_DUNGEON, "", 1},

    // Death Mountain Trail
    { 0x0191, 0x013D, SINGLE_SCENE_INFO(0x60), "Sendero",                        "Kakariko",                   ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_KAKARIKO,              ENTRANCE_TYPE_OVERWORLD},
    { 0x014D, 0x01B9, SINGLE_SCENE_INFO(0x60), "Sendero",                        "Ciudad Goron",                 ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_GORON_CITY,            ENTRANCE_TYPE_OVERWORLD, "gc"},
    { 0x0147, 0x01BD, SINGLE_SCENE_INFO(0x60), "Sendero",                        "Cráter",                        ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_TYPE_OVERWORLD},
    { 0x0315, 0x045B, SINGLE_SCENE_INFO(0x60), "Sendero",                        "Fuente Gran hada (Sendero)",   ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL,  ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x0708, 0x0808, SINGLE_SCENE_INFO(0x60), "Sendero",                        "Gruta de la tormenta (Sendero)",          ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL,  ENTRANCE_TYPE_GROTTO,    "chest", 1},
    { 0x0709, 0x0809, SINGLE_SCENE_INFO(0x60), "Sendero",                        "Gruta de la vaca (Sendero)",             ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL,  ENTRANCE_TYPE_GROTTO,    "", 1},
    { 0x0004, 0x0242, SINGLE_SCENE_INFO(0x60), "Sendero",                        "Cueva de los dodongos",           ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL,  ENTRANCE_TYPE_DUNGEON,   "dc", 1},
    { 0x045B, 0x0315, {{ 0x3B, 0x00 }},        "Fuente Gran hada (Sendero)",   "Sendero",                        ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL,  ENTRANCE_TYPE_INTERIOR},
    { 0x0808, 0x0708, {{ 0x3E, 0x00 }},        "Gruta de la tormenta (Sendero)",          "Sendero",                        ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL,  ENTRANCE_TYPE_GROTTO,    "chest"},
    { 0x0809, 0x0709, {{ 0x3E, 0x0D }},        "Gruta de la vaca (Sendero)",             "Sendero",                        ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL,  ENTRANCE_TYPE_GROTTO},
    { 0x0242, 0x0004, SINGLE_SCENE_INFO(0x01), "Cueva de los dodongos",           "Sendero",                        ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL,  ENTRANCE_TYPE_DUNGEON,   "dc"},
    { 0x040B, 0x00C5, SINGLE_SCENE_INFO(0x01), "Puerta del jefe de la Cueva de los dodongos", "Rey Dodongo",               ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL,  ENTRANCE_TYPE_DUNGEON,   "dc", 1},
    { 0x00C5, 0x040B, SINGLE_SCENE_INFO(0x12), "Rey Dodongo",               "Puerta del jefe de la Cueva de los dodongos", ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL,  ENTRANCE_TYPE_DUNGEON,   "dc", 1},

    // Death Mountain Crater
    { 0x01C1, 0x0246, SINGLE_SCENE_INFO(0x61), "Cráter",                      "Ciudad Goron",               ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_GROUP_GORON_CITY,            ENTRANCE_TYPE_OVERWORLD, "gc"},
    { 0x01BD, 0x0147, SINGLE_SCENE_INFO(0x61), "Cráter",                      "Sendero",                      ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL,  ENTRANCE_TYPE_OVERWORLD},
    { 0x04BE, 0x0482, SINGLE_SCENE_INFO(0x61), "Cráter",                      "Fuente Gran hada (Cráter)", ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x0706, 0x0806, SINGLE_SCENE_INFO(0x61), "Cráter",                      "Gruta alta (Cráter)",         ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_TYPE_GROTTO,    "chest", 1},
    { 0x0705, 0x0805, SINGLE_SCENE_INFO(0x61), "Cráter",                      "Gruta del martillo (Cráter)",        ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_TYPE_GROTTO,    "scrubs", 1},
    { 0x0165, 0x024A, SINGLE_SCENE_INFO(0x61), "Cráter",                      "Templo del Fuego",              ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_TYPE_DUNGEON,   "", 1},
    { 0x0482, 0x04BE, {{ 0x3B, 0x01 }},        "Fuente Gran hada (Cráter)", "Cráter",                      ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_TYPE_INTERIOR},
    { 0x0806, 0x0706, {{ 0x3E, 0x00 }},        "Gruta alta (Cráter)",         "Cráter",                      ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_TYPE_GROTTO,    "chest"},
    { 0x0805, 0x0705, {{ 0x3E, 0x04 }},        "Gruta del martillo (Cráter)",        "Cráter",                      ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_TYPE_GROTTO,    "scrubs"},
    { 0x024A, 0x0165, SINGLE_SCENE_INFO(0x04), "Templo del Fuego",              "Cráter",                      ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_TYPE_DUNGEON},
    { 0x0305, 0x0175, SINGLE_SCENE_INFO(0x04), "Puerta del jefe del Templo del Fuego",    "Volvagia",                 ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_TYPE_DUNGEON,   "", 1},
    { 0x0175, 0x0305, SINGLE_SCENE_INFO(0x15), "Volvagia",                 "Puerta del jefe del Templo del Fuego",    ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_TYPE_DUNGEON,   "", 1},

    // Goron City
    { 0x01B9, 0x014D, SINGLE_SCENE_INFO(0x62), "Ciudad Goron",        "Sendero",               ENTRANCE_GROUP_GORON_CITY, ENTRANCE_GROUP_DEATH_MOUNTAIN_TRAIL,  ENTRANCE_TYPE_OVERWORLD, "gc"},
    { 0x0246, 0x01C1, SINGLE_SCENE_INFO(0x62), "Ciudad Goron",        "Cráter",               ENTRANCE_GROUP_GORON_CITY, ENTRANCE_GROUP_DEATH_MOUNTAIN_CRATER, ENTRANCE_TYPE_OVERWORLD, "gc"},
    { 0x04D6, 0x04E2, SINGLE_SCENE_INFO(0x62), "Ciudad Goron",        "Bosque Perdido",        ENTRANCE_GROUP_GORON_CITY, ENTRANCE_GROUP_LOST_WOODS,            ENTRANCE_TYPE_OVERWORLD, "gc,lw"},
    { 0x037C, 0x03FC, SINGLE_SCENE_INFO(0x62), "Ciudad Goron",        "Tienda Goron",        ENTRANCE_GROUP_GORON_CITY, ENTRANCE_GROUP_GORON_CITY,            ENTRANCE_TYPE_INTERIOR,  "gc", 1},
    { 0x0707, 0x0807, SINGLE_SCENE_INFO(0x62), "Ciudad Goron",        "Gruta de Ciudad Goron", ENTRANCE_GROUP_GORON_CITY, ENTRANCE_GROUP_GORON_CITY,            ENTRANCE_TYPE_GROTTO,    "gc,scrubs", 1},
    { 0x03FC, 0x037C, SINGLE_SCENE_INFO(0x2E), "Tienda Goron",        "Ciudad Goron",        ENTRANCE_GROUP_GORON_CITY, ENTRANCE_GROUP_GORON_CITY,            ENTRANCE_TYPE_INTERIOR,  "gc"},
    { 0x0807, 0x0707, {{ 0x3E, 0x04 }},        "Gruta de Ciudad Goron", "Ciudad Goron",        ENTRANCE_GROUP_GORON_CITY, ENTRANCE_GROUP_GORON_CITY,            ENTRANCE_TYPE_GROTTO,    "gc,scrubs"},

    // Zora's River
    { 0x0181, 0x00EA, SINGLE_SCENE_INFO(0x54), "Río Zora",               "Llanura de Hyrule",     ENTRANCE_GROUP_ZORAS_RIVER, ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_TYPE_OVERWORLD, "hf"},
    { 0x04DA, 0x01DD, SINGLE_SCENE_INFO(0x54), "Río Zora",               "Bosque Perdido",       ENTRANCE_GROUP_ZORAS_RIVER, ENTRANCE_GROUP_LOST_WOODS,   ENTRANCE_TYPE_OVERWORLD, "lw"},
    { 0x0108, 0x019D, SINGLE_SCENE_INFO(0x54), "Río Zora",               "Región de los zora",    ENTRANCE_GROUP_ZORAS_RIVER, ENTRANCE_GROUP_ZORAS_DOMAIN, ENTRANCE_TYPE_OVERWORLD},
    { 0x0702, 0x0802, SINGLE_SCENE_INFO(0x54), "Río Zora",               "Gruta de la tormenta (Río Zora)", ENTRANCE_GROUP_ZORAS_RIVER, ENTRANCE_GROUP_ZORAS_RIVER,  ENTRANCE_TYPE_GROTTO,    "scrubs", 1},
    { 0x0703, 0x0803, SINGLE_SCENE_INFO(0x54), "Río Zora",               "Gruta de las hadas (Río Zora)",  ENTRANCE_GROUP_ZORAS_RIVER, ENTRANCE_GROUP_ZORAS_RIVER,  ENTRANCE_TYPE_GROTTO,    "", 1},
    { 0x0704, 0x0804, SINGLE_SCENE_INFO(0x54), "Río Zora",               "Gruta abierta (Río Zora)",   ENTRANCE_GROUP_ZORAS_RIVER, ENTRANCE_GROUP_ZORAS_RIVER,  ENTRANCE_TYPE_GROTTO,    "chest", 1},
    { 0x0802, 0x0702, {{ 0x3E, 0x0A }},        "Gruta de la tormenta (Río Zora)", "Río Zora",               ENTRANCE_GROUP_ZORAS_RIVER, ENTRANCE_GROUP_ZORAS_RIVER,  ENTRANCE_TYPE_GROTTO,    "scrubs"},
    { 0x0803, 0x0703, {{ 0x3C, 0x00 }},        "Gruta de las hadas (Río Zora)",  "Río Zora",               ENTRANCE_GROUP_ZORAS_RIVER, ENTRANCE_GROUP_ZORAS_RIVER,  ENTRANCE_TYPE_GROTTO},
    { 0x0804, 0x0704, {{ 0x3E, 0x00 }},        "Gruta abierta (Río Zora)",   "Río Zora",               ENTRANCE_GROUP_ZORAS_RIVER, ENTRANCE_GROUP_ZORAS_RIVER,  ENTRANCE_TYPE_GROTTO,    "chest"},

    // Zora's Domain
    { 0x019D, 0x0108, SINGLE_SCENE_INFO(0x58), "Región de los zora",    "Río Zora",               ENTRANCE_GROUP_ZORAS_DOMAIN, ENTRANCE_GROUP_ZORAS_RIVER,    ENTRANCE_TYPE_OVERWORLD},
    { 0x0560, 0x0328, SINGLE_SCENE_INFO(0x58), "Región de los zora",    "Lago Hylia",       ENTRANCE_GROUP_ZORAS_DOMAIN, ENTRANCE_GROUP_LAKE_HYLIA,     ENTRANCE_TYPE_OVERWORLD, "lh"},
    { 0x0225, 0x01A1, SINGLE_SCENE_INFO(0x58), "Región de los zora",    "Fuente Zora",               ENTRANCE_GROUP_ZORAS_DOMAIN, ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_TYPE_OVERWORLD},
    { 0x0380, 0x03C4, SINGLE_SCENE_INFO(0x58), "Región de los zora",    "Tienda Zora",        ENTRANCE_GROUP_ZORAS_DOMAIN, ENTRANCE_GROUP_ZORAS_DOMAIN,   ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x071C, 0x081C, SINGLE_SCENE_INFO(0x58), "Región de los zora",    "Gruta de la tormenta (Región Zora)", ENTRANCE_GROUP_ZORAS_DOMAIN, ENTRANCE_GROUP_ZORAS_DOMAIN,   ENTRANCE_TYPE_GROTTO,    "fairy", 1},
    { 0x03C4, 0x0380, SINGLE_SCENE_INFO(0x2F), "Tienda Zora",        "Región de los zora",    ENTRANCE_GROUP_ZORAS_DOMAIN, ENTRANCE_GROUP_ZORAS_DOMAIN,   ENTRANCE_TYPE_INTERIOR},
    { 0x081C, 0x071C, {{ 0x3C, 0x00 }},        "Gruta de la tormenta (Región Zora)", "Región de los zora",    ENTRANCE_GROUP_ZORAS_DOMAIN, ENTRANCE_GROUP_ZORAS_DOMAIN,   ENTRANCE_TYPE_GROTTO,    "fairy"},

    // Zora's Fountain
    { 0x01A1, 0x0225, SINGLE_SCENE_INFO(0x59), "Fuente Zora",                          "Región de los zora",               ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_GROUP_ZORAS_DOMAIN,   ENTRANCE_TYPE_OVERWORLD},
    { 0x0371, 0x0394, SINGLE_SCENE_INFO(0x59), "Fuente Zora",                          "Fuente Gran hada (Fuente Zora)",     ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_TYPE_INTERIOR, "", 1},
    { 0x0028, 0x0221, SINGLE_SCENE_INFO(0x59), "Fuente Zora",                          "Tripa de Jabu-Jabu",           ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_TYPE_DUNGEON,  "", 1},
    { 0x0088, 0x03D4, SINGLE_SCENE_INFO(0x59), "Fuente Zora",                          "Caverna de hielo",                  ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_TYPE_DUNGEON,  "", 1},
    { 0x0394, 0x0371, {{ 0x3D, 0x00 }},        "Fuente Gran hada (Fuente Zora)",     "Fuente Zora",                          ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_TYPE_INTERIOR},
    { 0x0221, 0x0028, SINGLE_SCENE_INFO(0x02), "Tripa de Jabu-Jabu",           "Fuente Zora",                          ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_TYPE_DUNGEON},
    { 0x0301, 0x0407, SINGLE_SCENE_INFO(0x02), "Puerta del jefe de la tripa de Jabu-Jabu", "Barinade",                    ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_TYPE_DUNGEON, "", 1},
    { 0x0407, 0x0301, SINGLE_SCENE_INFO(0x13), "Barinade",                    "Puerta del jefe de la tripa de Jabu-Jabu", ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_TYPE_DUNGEON, "", 1},
    { 0x03D4, 0x0088, SINGLE_SCENE_INFO(0x09), "Caverna de hielo",                  "Fuente Zora",                          ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_GROUP_ZORAS_FOUNTAIN, ENTRANCE_TYPE_DUNGEON},

    // Hyrule Field
    { 0x04DE, 0x0185, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Puente del Bosque Perdido",      ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_LOST_WOODS,    ENTRANCE_TYPE_OVERWORLD, "hf,lw"},
    { 0x0276, 0x01FD, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Entrada de la plaza",        ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_OVERWORLD, "hf"},
    { 0x0157, 0x01F9, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Rancho Lon Lon",          ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_TYPE_OVERWORLD, "hf,llr"},
    { 0x00DB, 0x017D, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Kakariko",               ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_KAKARIKO,      ENTRANCE_TYPE_OVERWORLD, "hf"},
    { 0x00EA, 0x0181, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Río Zora",                     ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_ZORAS_RIVER,   ENTRANCE_TYPE_OVERWORLD, "hf"},
    { 0x0102, 0x0189, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Lago Hylia",             ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_LAKE_HYLIA,    ENTRANCE_TYPE_OVERWORLD, "hf,lh"},
    { 0x0117, 0x018D, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Valle Gerudo",                     ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_TYPE_OVERWORLD, "hf"},
    { 0x0710, 0x0810, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Gruta cerca de la plaza (Llanura)",  ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "chest", 1},
    { 0x070E, 0x080E, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Gruta cerca de Kakariko (Llanura)",     ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "spider", 1},
    { 0x070D, 0x080D, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Gruta tektite (Llanura)",      ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "water", 1},
    { 0x070F, 0x080F, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Gruta de las hadas (Llanura)",        ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "", 1},
    { 0x0711, 0x0811, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Gruta de la vaca (Llanura)",          ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "webbed", 1},
    { 0x0713, 0x0813, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Gruta abierta (Llanura)",         ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "chest", 1},
    { 0x0712, 0x0812, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Gruta tras la valla (Llanura)", ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "scrubs", 1},
    { 0x0714, 0x0814, SINGLE_SCENE_INFO(0x51), "Llanura de Hyrule",           "Gruta sureste (Llanura)",    ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "chest", 1},
    { 0x0810, 0x0710, {{ 0x3E, 0x00 }},        "Gruta cerca de la plaza (Llanura)",  "Llanura de Hyrule",           ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO},
    { 0x080E, 0x070E, {{ 0x3E, 0x01 }},        "Gruta cerca de Kakariko (Llanura)",     "Llanura de Hyrule",           ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "spider"},
    { 0x080D, 0x070D, {{ 0x3E, 0x0B }},        "Gruta tektite (Llanura)",      "Llanura de Hyrule",           ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "water"},
    { 0x080F, 0x070F, {{ 0x3C, 0x00 }},        "Gruta de las hadas (Llanura)",        "Llanura de Hyrule",           ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO},
    { 0x0811, 0x0711, {{ 0x3E, 0x05 }},        "Gruta de la vaca (Llanura)",          "Llanura de Hyrule",           ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "webbed"},
    { 0x0813, 0x0713, {{ 0x3E, 0x00 }},        "Gruta abierta (Llanura)",         "Llanura de Hyrule",           ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "chest"},
    { 0x0812, 0x0712, {{ 0x3E, 0x02 }},        "Gruta tras la valla (Llanura)", "Llanura de Hyrule",           ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "srubs"},
    { 0x0814, 0x0714, {{ 0x3E, 0x00 }},        "Gruta sureste (Llanura)",    "Llanura de Hyrule",           ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_GROTTO,    "chest"},

    // Lon Lon Ranch
    { 0x01F9, 0x0157, SINGLE_SCENE_INFO(0x63), "Rancho Lon Lon", "Llanura de Hyrule",  ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_OVERWORLD, "hf"},
    { 0x004F, 0x0378, SINGLE_SCENE_INFO(0x63), "Rancho Lon Lon", "Casa de Talon", ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_TYPE_INTERIOR,  "llr", 1},
    { 0x02F9, 0x042F, SINGLE_SCENE_INFO(0x63), "Rancho Lon Lon", "Establos del Rancho",   ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_TYPE_INTERIOR,  "cow", 1},
    { 0x05D0, 0x05D4, SINGLE_SCENE_INFO(0x63), "Rancho Lon Lon", "Torre del Rancho",     ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_TYPE_INTERIOR,  "cow", 1},
    { 0x0715, 0x0815, SINGLE_SCENE_INFO(0x63), "Rancho Lon Lon", "Gruta del Rancho",    ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_TYPE_GROTTO,    "scrubs", 1},
    { 0x0378, 0x004F, {{ 0x4C, 0x00 }},        "Casa de Talon", "Rancho Lon Lon", ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_TYPE_INTERIOR,  "llr"},
    { 0x042F, 0x02F9, SINGLE_SCENE_INFO(0x36), "Establos del Rancho",   "Rancho Lon Lon", ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_TYPE_INTERIOR,  "cow"},
    { 0x05D4, 0x05D0, {{ 0x4C, 0x01 }},        "Torre del Rancho",     "Rancho Lon Lon", ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_TYPE_INTERIOR,  "cow"},
    { 0x0815, 0x0715, {{ 0x3E, 0x04 }},        "Gruta del Rancho",    "Rancho Lon Lon", ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_GROUP_LON_LON_RANCH, ENTRANCE_TYPE_GROTTO,    "scrubs"},

    // Lake Hylia
    { 0x0189, 0x0102, SINGLE_SCENE_INFO(0x57), "Lago Hylia",             "Llanura de Hyrule",           ENTRANCE_GROUP_LAKE_HYLIA, ENTRANCE_GROUP_HYRULE_FIELD, ENTRANCE_TYPE_OVERWORLD, "lh"},
    { 0x0328, 0x0560, SINGLE_SCENE_INFO(0x57), "Lago Hylia",             "Región de los zora",          ENTRANCE_GROUP_LAKE_HYLIA, ENTRANCE_GROUP_ZORAS_DOMAIN, ENTRANCE_TYPE_OVERWORLD, "lh"},
    { 0x0043, 0x03CC, SINGLE_SCENE_INFO(0x57), "Lago Hylia",             "Laboratorio del lago",                 ENTRANCE_GROUP_LAKE_HYLIA, ENTRANCE_GROUP_LAKE_HYLIA,   ENTRANCE_TYPE_INTERIOR,  "lh", 1},
    { 0x045F, 0x0309, SINGLE_SCENE_INFO(0x57), "Lago Hylia",             "Estanque de pesca",           ENTRANCE_GROUP_LAKE_HYLIA, ENTRANCE_GROUP_LAKE_HYLIA,   ENTRANCE_TYPE_INTERIOR,  "lh", 1},
    { 0x0701, 0x0801, SINGLE_SCENE_INFO(0x57), "Lago Hylia",             "Gruta del Lago",              ENTRANCE_GROUP_LAKE_HYLIA, ENTRANCE_GROUP_LAKE_HYLIA,   ENTRANCE_TYPE_GROTTO,    "scrubs", 1},
    { 0x0010, 0x021D, SINGLE_SCENE_INFO(0x57), "Lago Hylia",             "Templo del Agua",           ENTRANCE_GROUP_LAKE_HYLIA, ENTRANCE_GROUP_LAKE_HYLIA,   ENTRANCE_TYPE_DUNGEON,   "lh", 1},
    { 0x03CC, 0x0043, SINGLE_SCENE_INFO(0x38), "Laboratorio del lago",                 "Lago Hylia",             ENTRANCE_GROUP_LAKE_HYLIA, ENTRANCE_GROUP_LAKE_HYLIA,   ENTRANCE_TYPE_INTERIOR,  "lh"},
    { 0x0309, 0x045F, SINGLE_SCENE_INFO(0x49), "Estanque de pesca",           "Lago Hylia",             ENTRANCE_GROUP_LAKE_HYLIA, ENTRANCE_GROUP_LAKE_HYLIA,   ENTRANCE_TYPE_INTERIOR,  "lh"},
    { 0x0801, 0x0701, {{ 0x3E, 0x04 }},        "Gruta del Lago",              "Lago Hylia",             ENTRANCE_GROUP_LAKE_HYLIA, ENTRANCE_GROUP_LAKE_HYLIA,   ENTRANCE_TYPE_GROTTO,    "lh,scrubs"},
    { 0x021D, 0x0010, SINGLE_SCENE_INFO(0x05), "Templo del Agua",           "Lago Hylia",             ENTRANCE_GROUP_LAKE_HYLIA, ENTRANCE_GROUP_LAKE_HYLIA,   ENTRANCE_TYPE_DUNGEON,   "lh"},
    { 0x0417, 0x0423, SINGLE_SCENE_INFO(0x05), "Puerta del jefe del Templo del Agua", "Morpha",                 ENTRANCE_GROUP_LAKE_HYLIA, ENTRANCE_GROUP_LAKE_HYLIA,   ENTRANCE_TYPE_DUNGEON,   "lh", 1},
    { 0x0423, 0x0417, SINGLE_SCENE_INFO(0x16), "Morpha",                 "Puerta del jefe del Templo del Agua", ENTRANCE_GROUP_LAKE_HYLIA, ENTRANCE_GROUP_LAKE_HYLIA,   ENTRANCE_TYPE_DUNGEON,   "lh", 1},

    // Gerudo Area
    { 0x018D, 0x0117, SINGLE_SCENE_INFO(0x5A), "Valle Gerudo",                      "Llanura de Hyrule",            ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_HYRULE_FIELD,      ENTRANCE_TYPE_OVERWORLD, "hf"},
    { 0x0129, 0x022D, SINGLE_SCENE_INFO(0x5A), "Valle Gerudo",                      "Fortaleza gerudo",                      ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_OVERWORLD, "gerudo fortress"},
    { 0x0219, -1,     SINGLE_SCENE_INFO(0x5A), "Valle Gerudo",                      "Lago Hylia",              ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_LAKE_HYLIA,        ENTRANCE_TYPE_OVERWORLD, "lh"},
    { 0x03A0, 0x03D0, SINGLE_SCENE_INFO(0x5A), "Valle Gerudo",                      "Tienda de los carpinteros",        ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x071F, 0x081F, SINGLE_SCENE_INFO(0x5A), "Valle Gerudo",                      "Gruta octorok (Valle)",       ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_GROTTO,    "", 1},
    { 0x071E, 0x081E, SINGLE_SCENE_INFO(0x5A), "Valle Gerudo",                      "Gruta de la tormenta (Valle)",        ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_GROTTO,    "scrubs", 1},
    { 0x022D, 0x0129, SINGLE_SCENE_INFO(0x5D), "Fortaleza gerudo",                      "Valle Gerudo",                      ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_OVERWORLD, "gerudo fortress"},
    { 0x0130, 0x03AC, SINGLE_SCENE_INFO(0x5D), "Fortaleza gerudo",                      "Desierto encantado",       ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_TYPE_OVERWORLD, "gerudo fortress"},
    { 0x071D, 0x081D, SINGLE_SCENE_INFO(0x5D), "Fortaleza gerudo",                      "Gruta de la tormenta (Fortaleza)",        ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_GROTTO,    "gerudo fortress", 1},
    { 0x0008, 0x03A8, SINGLE_SCENE_INFO(0x5D), "Fortaleza gerudo",                      "Centro de Instrucción Gerudo", ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_DUNGEON,   "gerudo fortress,gtg", 1},
    { 0x03D0, 0x03A0, SINGLE_SCENE_INFO(0x39), "Tienda de los carpinteros",        "Valle Gerudo",                      ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_INTERIOR},
    { 0x081F, 0x071F, {{ 0x3E, 0x06 }},        "Gruta octorok (Valle)",       "Valle Gerudo",                      ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_GROTTO},
    { 0x081E, 0x071E, {{ 0x3E, 0x0A }},        "Gruta de la tormenta (Valle)",        "Valle Gerudo",                      ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_GROTTO,    "scrubs"},
    { 0x081D, 0x071D, {{ 0x3C, 0x00 }},        "Gruta de la tormenta (Fortaleza)",        "Fortaleza gerudo",                      ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_GROTTO,    "gerudo fortress"},
    { 0x03A8, 0x0008, SINGLE_SCENE_INFO(0x0B), "Centro de Instrucción Gerudo", "Fortaleza gerudo",                      ENTRANCE_GROUP_GERUDO_VALLEY, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_DUNGEON,   "gerudo fortress,gtg"},

    // The Wasteland
    { 0x03AC, 0x0130, SINGLE_SCENE_INFO(0x5E), "Desierto encantado",             "Fortaleza gerudo",                            ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_GROUP_GERUDO_VALLEY,     ENTRANCE_TYPE_OVERWORLD, "hw,gerudo fortress"},
    { 0x0123, 0x0365, SINGLE_SCENE_INFO(0x5E), "Desierto encantado",             "Coloso del desierto",               ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_TYPE_OVERWORLD, "dc,hw"},
    { 0x0365, 0x0123, SINGLE_SCENE_INFO(0x5C), "Coloso del desierto",               "Desierto encantado",             ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_TYPE_OVERWORLD, "dc,hw"},
    { 0x0588, 0x057C, SINGLE_SCENE_INFO(0x5C), "Coloso",                      "Fuente Gran hada (Coloso)", ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_TYPE_INTERIOR,  "dc", 1},
    { 0x0700, 0x0800, SINGLE_SCENE_INFO(0x5C), "Coloso del desierto",               "Gruta del Coloso",               ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_TYPE_GROTTO,    "dc,scrubs", 1},
    { 0x0082, 0x01E1, SINGLE_SCENE_INFO(0x5C), "Coloso del desierto",               "Templo del Espíritu",                 ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_TYPE_DUNGEON,   "dc", 1},
    { 0x057C, 0x0588, {{ 0x3D, 0x02 }},        "Fuente Gran hada (Coloso)", "Coloso",                      ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_TYPE_INTERIOR,  "dc"},
    { 0x0800, 0x0700, {{ 0x3E, 0x0A }},        "Gruta del Coloso",               "Coloso del desierto",               ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_TYPE_GROTTO,    "dc,scrubs"},
    { 0x01E1, 0x0082, SINGLE_SCENE_INFO(0x06), "Templo del Espíritu",                 "Coloso del desierto",               ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_TYPE_DUNGEON,   "dc"},
    { 0x008D, 0x02F5, SINGLE_SCENE_INFO(0x06), "Puerta del jefe del Templo del Espíritu",       "Twinrova",                      ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_TYPE_DUNGEON,   "", 1},
    { 0x02F5, 0x008D, SINGLE_SCENE_INFO(0x17), "Twinrova",                      "Puerta del jefe del Templo del Espíritu",       ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_GROUP_HAUNTED_WASTELAND, ENTRANCE_TYPE_DUNGEON,   "", 1},

    // Market
    { 0x01FD, 0x0276, {SCENE_NO_SPAWN(0x1B), SCENE_NO_SPAWN(0x1C), SCENE_NO_SPAWN(0x1D)}, "Entrada de la plaza",   "Llanura de Hyrule",        ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_HYRULE_FIELD,  ENTRANCE_TYPE_OVERWORLD, "hf"},
    { 0x00B1, 0x0033, {SCENE_NO_SPAWN(0x1B), SCENE_NO_SPAWN(0x1C), SCENE_NO_SPAWN(0x1D)}, "Entrada de la plaza",   "Plaza del mercado",              ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_OVERWORLD},
    { 0x007E, 0x026E, {SCENE_NO_SPAWN(0x1B), SCENE_NO_SPAWN(0x1C), SCENE_NO_SPAWN(0x1D)}, "Entrada de la plaza",   "Casa del guardia",         ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_INTERIOR,  "pots,poe", 1},
    { 0x0033, 0x00B1, {SCENE_NO_SPAWN(0x20), SCENE_NO_SPAWN(0x21), SCENE_NO_SPAWN(0x22), SCENE_NO_SPAWN(0x1E), SCENE_NO_SPAWN(0x1F)}, "Plaza del mercado",              "Entrada de la plaza",        ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_OVERWORLD},
    { 0x0138, 0x025A, {SCENE_NO_SPAWN(0x20), SCENE_NO_SPAWN(0x21), SCENE_NO_SPAWN(0x22), SCENE_NO_SPAWN(0x1E), SCENE_NO_SPAWN(0x1F)}, "Plaza del mercado",              "Jardines del Castillo / Ext. Ganon",       ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_TYPE_OVERWORLD, "outside ganon's castle"},
    { 0x0171, 0x025E, {SCENE_NO_SPAWN(0x20), SCENE_NO_SPAWN(0x21), SCENE_NO_SPAWN(0x22), SCENE_NO_SPAWN(0x1E), SCENE_NO_SPAWN(0x1F)}, "Plaza del mercado",              "Exterior del Templo del Tiempo", ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_OVERWORLD},
    { 0x016D, 0x01CD, {SCENE_NO_SPAWN(0x20), SCENE_NO_SPAWN(0x21), SCENE_NO_SPAWN(0x22), SCENE_NO_SPAWN(0x1E), SCENE_NO_SPAWN(0x1F)}, "Plaza del mercado",              "Galería de tiro de la plaza",    ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_INTERIOR,  "child", 1},
    { 0x0507, 0x03BC, {SCENE_NO_SPAWN(0x20), SCENE_NO_SPAWN(0x21), SCENE_NO_SPAWN(0x22), SCENE_NO_SPAWN(0x1E), SCENE_NO_SPAWN(0x1F)}, "Plaza del mercado",              "Bolera de bombuchus",        ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x0063, 0x01D5, {SCENE_NO_SPAWN(0x20), SCENE_NO_SPAWN(0x21), SCENE_NO_SPAWN(0x22), SCENE_NO_SPAWN(0x1E), SCENE_NO_SPAWN(0x1F)}, "Plaza del mercado",              "Juego de los cofres",    ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x043B, 0x0067, {SCENE_NO_SPAWN(0x20), SCENE_NO_SPAWN(0x21), SCENE_NO_SPAWN(0x22), SCENE_NO_SPAWN(0x1E), SCENE_NO_SPAWN(0x1F)}, "Plaza del mercado",              "Casa del hombre de verde",   ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x0530, 0x01D1, {SCENE_NO_SPAWN(0x20), SCENE_NO_SPAWN(0x21), SCENE_NO_SPAWN(0x22), SCENE_NO_SPAWN(0x1E), SCENE_NO_SPAWN(0x1F)}, "Plaza del mercado",              "Tienda de máscaras",              ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x052C, 0x03B8, {SCENE_NO_SPAWN(0x20), SCENE_NO_SPAWN(0x21), SCENE_NO_SPAWN(0x22), SCENE_NO_SPAWN(0x1E), SCENE_NO_SPAWN(0x1F)}, "Plaza del mercado",              "Bazar de la plaza",              ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_INTERIOR,  "shop", 1},
    { 0x0388, 0x02A2, {SCENE_NO_SPAWN(0x20), SCENE_NO_SPAWN(0x21), SCENE_NO_SPAWN(0x22), SCENE_NO_SPAWN(0x1E), SCENE_NO_SPAWN(0x1F)}, "Plaza del mercado",              "Tienda de pociones de la plaza",         ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x0528, 0x03C0, {SCENE_NO_SPAWN(0x20), SCENE_NO_SPAWN(0x21), SCENE_NO_SPAWN(0x22), SCENE_NO_SPAWN(0x1E), SCENE_NO_SPAWN(0x1F)}, "Plaza del mercado",              "Tienda de bombuchus",           ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_INTERIOR,  "", 1},
    { 0x026E, 0x007E, {{ 0x4D }},              "Casa del guardia",                             "Entrada de la plaza",        ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,      ENTRANCE_TYPE_INTERIOR, "pots,poe"},
    { 0x01CD, 0x016D, {{ 0x42, 0x01 }},        "Galería de tiro de la plaza",                     "Plaza del mercado",                 ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,      ENTRANCE_TYPE_INTERIOR},
    { 0x03BC, 0x0507, SINGLE_SCENE_INFO(0x4B), "Bolera de bombuchus",                         "Plaza del mercado",                 ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,      ENTRANCE_TYPE_INTERIOR},
    { 0x01D5, 0x0063, SINGLE_SCENE_INFO(0x10), "Juego de los cofres",                     "Plaza del mercado",                 ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,      ENTRANCE_TYPE_INTERIOR},
    { 0x0067, 0x043B, SINGLE_SCENE_INFO(0x2B), "Casa del hombre de verde",                    "Plaza del mercado",                 ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,      ENTRANCE_TYPE_INTERIOR},
    { 0x01D1, 0x0530, SINGLE_SCENE_INFO(0x33), "Tienda de máscaras",                               "Plaza del mercado",                 ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,      ENTRANCE_TYPE_INTERIOR},
    { 0x03B8, 0x052C, {{ 0x2C, 0x01 }},        "Bazar de la plaza",                               "Plaza del mercado",                 ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,      ENTRANCE_TYPE_INTERIOR, "shop"},
    { 0x02A2, 0x0388, SINGLE_SCENE_INFO(0x31), "Tienda de pociones de la plaza",                          "Plaza del mercado",                 ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,      ENTRANCE_TYPE_INTERIOR},
    { 0x03C0, 0x0528, SINGLE_SCENE_INFO(0x32), "Tienda de bombuchus",                            "Plaza del mercado",                 ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,      ENTRANCE_TYPE_INTERIOR},
    { 0x025E, 0x0171, {SCENE_NO_SPAWN(0x23), SCENE_NO_SPAWN(0x24), SCENE_NO_SPAWN(0x25)}, "Exterior del Templo del Tiempo", "Plaza del mercado",                ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_OVERWORLD, "tot"},
    { 0x0053, 0x0472, {SCENE_NO_SPAWN(0x23), SCENE_NO_SPAWN(0x24), SCENE_NO_SPAWN(0x25)}, "Exterior del Templo del Tiempo", "Templo del Tiempo",        ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,        ENTRANCE_TYPE_INTERIOR, "tot", 1},
    { 0x0472, 0x0053, SINGLE_SCENE_INFO(0x43), "Templo del Tiempo",                          "Exterior del Templo del Tiempo", ENTRANCE_GROUP_MARKET, ENTRANCE_GROUP_MARKET,      ENTRANCE_TYPE_INTERIOR, "tot"},

    // Hyrule Castle
    { 0x025A, 0x0138, {SCENE_NO_SPAWN(0x5F), SCENE_NO_SPAWN(0x64)},        "Jardines del Castillo / Ext. Ganon",         "Plaza del mercado",                     ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_GROUP_MARKET, ENTRANCE_TYPE_OVERWORLD, "outside ganon's castle"},
    { 0x0578, 0x0340, SINGLE_SCENE_INFO(0x5F), "Jardines del Castillo",               "Fuente Gran hada (Castillo)",  ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_TYPE_INTERIOR, "", 1},
    { 0x070C, 0x080C, SINGLE_SCENE_INFO(0x5F), "Jardines del Castillo",               "Gruta de la tormenta (Castillo)",         ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_TYPE_GROTTO,   "bombable", 1},
    { 0x0340, 0x0578, {{ 0x3D, 0x01 }},        "Fuente Gran hada (Castillo)",  "Jardines del Castillo",               ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_TYPE_INTERIOR},
    { 0x080C, 0x070C, {{ 0x3E, 0x09 }},        "Gruta de la tormenta (Castillo)",         "Jardines del Castillo",               ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_TYPE_GROTTO,   "bombable"},
    { 0x04C2, 0x03E8, SINGLE_SCENE_INFO(0x64), "Ext. Castillo de Ganon",                      "Fuente Gran hada (Ext. Ganon)", ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_TYPE_INTERIOR, "outside ganon's castle", 1},
    { 0x0467, 0x023D, SINGLE_SCENE_INFO(0x64), "Ext. Castillo de Ganon",                      "Castillo de Ganon",           ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_TYPE_DUNGEON,  "outside ganon's castle,gc", 1},
    { 0x03E8, 0x04C2, {{ 0x3B, 0x02 }},        "Fuente Gran hada (Ext. Ganon)", "Ext. Castillo de Ganon",                      ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_TYPE_INTERIOR, "outside ganon's castle"},
    { 0x023D, 0x0467, SINGLE_SCENE_INFO(0x0D), "Castillo de Ganon",           "Ext. Castillo de Ganon",                      ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_GROUP_HYRULE_CASTLE, ENTRANCE_TYPE_DUNGEON,  "outside ganon's castle,gc"}
};

// Check if Link is in the area and return that scene/entrance for tracking
s8 LinkIsInArea(const EntranceData* entrance) {
    bool result = false;

    if (gPlayState == nullptr) {
        return -1;
    }

    // Handle detecting the current grotto
    if ((gPlayState->sceneNum == SCENE_YOUSEI_IZUMI_TATE || gPlayState->sceneNum == SCENE_KAKUSIANA) &&
        entrance->type == ENTRANCE_TYPE_GROTTO) {
        if (entrance->index == (ENTRANCE_RANDO_GROTTO_EXIT_START + currentGrottoId)) {
            // Return the grotto entrance for tracking
            return entrance->index;
        } else {
            return -1;
        }
    }

    // Otherwise check all scenes/spawns
    // Not all areas require a spawn position to differeniate between another area
    for (auto info : entrance->scenes) {
        // When a spawn position is specified, check that combination
        if (info.spawn != -1) {
            result = Entrance_SceneAndSpawnAre(info.scene, info.spawn);
        } else { // Otherwise just check the current scene
            result = gPlayState->sceneNum == info.scene;
        }

        // Return the scene for tracking
        if (result) {
            return info.scene;
        }
    }

    return -1;
}

bool IsEntranceDiscovered(s16 index) {
    bool isDiscovered = Entrance_GetIsEntranceDiscovered(index);
    if (!isDiscovered) {
        // If the pair included one of the hyrule field <-> zora's river entrances,
        // the randomizer will have also overriden the water-based entrances, so check those too
        if ((index == 0x00EA && Entrance_GetIsEntranceDiscovered(0x01D9)) || (index == 0x01D9 && Entrance_GetIsEntranceDiscovered(0x00EA))) {
            isDiscovered = true;
        } else if ((index == 0x0181 && Entrance_GetIsEntranceDiscovered(0x0311)) || (index == 0x0311 && Entrance_GetIsEntranceDiscovered(0x0181))) {
            isDiscovered = true;
        }
    }
    return isDiscovered;
}

const EntranceData* GetEntranceData(s16 index) {
    for (size_t i = 0; i < ARRAY_COUNT(entranceData); i++) {
        if (index == entranceData[i].index) {
            return &entranceData[i];
        }
    }
    // Shouldn't be reached
    return nullptr;
}

void SortEntranceListByType(EntranceOverride* entranceList, u8 byDest) {
    EntranceOverride tempList[ENTRANCE_OVERRIDES_MAX_COUNT] = { 0 };

    for (size_t i = 0; i < ENTRANCE_OVERRIDES_MAX_COUNT; i++) {
        tempList[i] = entranceList[i];
    }

    size_t idx = 0;

    for (size_t k = 0; k < ENTRANCE_TYPE_COUNT; k++) {
        for (size_t i = 0; i < ARRAY_COUNT(entranceData); i++) {
            for (size_t j = 0; j < ENTRANCE_OVERRIDES_MAX_COUNT; j++) {
                if (Entrance_EntranceIsNull(&tempList[j])) {
                    break;
                }

                size_t entranceIndex = byDest ? tempList[j].override : tempList[j].index;

                if (entranceData[i].type == k && entranceIndex == entranceData[i].index) {
                    entranceList[idx] = tempList[j];
                    idx++;
                    break;
                }
            }
        }
    }
}

void SortEntranceListByArea(EntranceOverride* entranceList, u8 byDest) {
    EntranceOverride tempList[ENTRANCE_OVERRIDES_MAX_COUNT] = { 0 };

    // Store to temp
    for (size_t i = 0; i < ENTRANCE_OVERRIDES_MAX_COUNT; i++) {
        tempList[i] = entranceList[i];
        // Don't include one-way indexes in the tempList if we're sorting by destination
        // so that we keep them at the beginning.
        if (byDest) {
            if (GetEntranceData(tempList[i].index)->srcGroup == ENTRANCE_GROUP_ONE_WAY) {
                tempList[i] = emptyOverride;
            }
        }
    }

    size_t idx = 0;
    // Sort Source List based on entranceData order
    if (!byDest) {
        for (size_t i = 0; i < ARRAY_COUNT(entranceData); i++) {
            for (size_t j = 0; j < ENTRANCE_OVERRIDES_MAX_COUNT; j++) {
                if (Entrance_EntranceIsNull(&tempList[j])) {
                    break;
                }
                if (tempList[j].index == entranceData[i].index) {
                    entranceList[idx] = tempList[j];
                    idx++;
                    break;
                }
            }
        }

    } else {
        // Increment the idx by however many one-way entrances are shuffled since these
        // will still be displayed at the beginning
        idx += gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_SOURCE_AREA][ENTRANCE_GROUP_ONE_WAY];

        // Sort the rest of the Destination List by matching destination strings with source strings when possible
        // and otherwise by group
        for (size_t group = ENTRANCE_GROUP_KOKIRI_FOREST; group < SPOILER_ENTRANCE_GROUP_COUNT; group++) {
            for (size_t i = 0; i < ENTRANCE_OVERRIDES_MAX_COUNT; i++) {
                if (Entrance_EntranceIsNull(&gSaveContext.entranceOverrides[i])) {
                    continue;
                }
                const EntranceData* curEntrance = GetEntranceData(gSaveContext.entranceOverrides[i].index);
                if (curEntrance->srcGroup != group) {
                    continue;
                }
                // First, search the list for the matching reverse entrance if it exists
                for (size_t j = 0; j < ENTRANCE_OVERRIDES_MAX_COUNT; j++) {
                    const EntranceData* curOverride = GetEntranceData(tempList[j].override);
                    if (Entrance_EntranceIsNull(&tempList[j]) || curOverride->dstGroup != group) {
                        continue;
                    }

                    if (curEntrance->reverseIndex == curOverride->index) {
                        entranceList[idx] = tempList[j];
                        // "Remove" this entrance from the tempList by setting it's values to zero
                        tempList[j] = emptyOverride;
                        idx++;
                        break;
                    }
                }
            }
            // Then find any remaining entrances in the same group and add them to the end
            for (size_t i = 0; i < ENTRANCE_OVERRIDES_MAX_COUNT; i++) {
                if (Entrance_EntranceIsNull(&tempList[i])) {
                    continue;
                }
                const EntranceData* curOverride = GetEntranceData(tempList[i].override);
                if (curOverride->dstGroup == group) {
                    entranceList[idx] = tempList[i];
                    tempList[i] = emptyOverride;
                    idx++;
                }
            }
        }
    }
}

void SetCurrentGrottoIDForTracker(s16 entranceIndex) {
    currentGrottoId = entranceIndex;
}

void SetLastEntranceOverrideForTracker(s16 entranceIndex) {
    lastEntranceIndex = entranceIndex;
}

void ClearEntranceTrackingData() {
    currentGrottoId = -1;
    lastEntranceIndex = -1;
    lastSceneOrEntranceDetected = -1;
    gEntranceTrackingData = {0};
}

void InitEntranceTrackingData() {
    gEntranceTrackingData = {0};

    // Check if entrance randomization is disabled
    if (!OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_ENTRANCES)) {
        return;
    }

    // Set total and group counts
    for (size_t i = 0; i < ENTRANCE_OVERRIDES_MAX_COUNT; i++) {
        if (Entrance_EntranceIsNull(&gSaveContext.entranceOverrides[i])) {
            break;
        }
        const EntranceData* index = GetEntranceData(gSaveContext.entranceOverrides[i].index);
        const EntranceData* override = GetEntranceData(gSaveContext.entranceOverrides[i].override);

        if (index->srcGroup == ENTRANCE_GROUP_ONE_WAY) {
            gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_SOURCE_AREA][ENTRANCE_GROUP_ONE_WAY]++;
            gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_DESTINATION_AREA][ENTRANCE_GROUP_ONE_WAY]++;
            gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_SOURCE_TYPE][ENTRANCE_TYPE_ONE_WAY]++;
            gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_DESTINATION_TYPE][ENTRANCE_TYPE_ONE_WAY]++;
        } else {
            gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_SOURCE_AREA][index->srcGroup]++;
            gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_DESTINATION_AREA][override->dstGroup]++;
            gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_SOURCE_TYPE][index->type]++;
            gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_DESTINATION_TYPE][override->type]++;
        }
        gEntranceTrackingData.EntranceCount++;
    }

    // The entrance data is sorted and grouped in a one dimensional array, so we need to track offsets
    // Set offsets for areas starting at 0
    u16 srcOffsetTotal = 0;
    u16 dstOffsetTotal = 0;
    for (size_t i = 0; i < SPOILER_ENTRANCE_GROUP_COUNT; i++) {
        // Set the offset for the current group
        gEntranceTrackingData.GroupOffsets[ENTRANCE_SOURCE_AREA][i] = srcOffsetTotal;
        gEntranceTrackingData.GroupOffsets[ENTRANCE_DESTINATION_AREA][i] = dstOffsetTotal;
        // Increment the offset by the areas entrance count
        srcOffsetTotal += gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_SOURCE_AREA][i];
        dstOffsetTotal += gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_DESTINATION_AREA][i];
    }
    // Set offsets for types starting at 0
    srcOffsetTotal = 0;
    dstOffsetTotal = 0;
    for (size_t i = 0; i < ENTRANCE_TYPE_COUNT; i++) {
        // Set the offset for the current group
        gEntranceTrackingData.GroupOffsets[ENTRANCE_SOURCE_TYPE][i] = srcOffsetTotal;
        gEntranceTrackingData.GroupOffsets[ENTRANCE_DESTINATION_TYPE][i] = dstOffsetTotal;
        // Increment the offset by the areas entrance count
        srcOffsetTotal += gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_SOURCE_TYPE][i];
        dstOffsetTotal += gEntranceTrackingData.GroupEntranceCounts[ENTRANCE_DESTINATION_TYPE][i];
    }

    // Sort entrances by group and type in entranceData
    for (size_t i = 0; i < ENTRANCE_OVERRIDES_MAX_COUNT; i++) {
        srcListSortedByArea[i] = gSaveContext.entranceOverrides[i];
        destListSortedByArea[i] = gSaveContext.entranceOverrides[i];
        srcListSortedByType[i] = gSaveContext.entranceOverrides[i];
        destListSortedByType[i] = gSaveContext.entranceOverrides[i];
    }
    SortEntranceListByArea(srcListSortedByArea, 0);
    SortEntranceListByArea(destListSortedByArea, 1);
    SortEntranceListByType(srcListSortedByType, 0);
    SortEntranceListByType(destListSortedByType, 1);
}

void DrawEntranceTracker(bool& open) {
    if (!open) {
        CVarSetInteger("gEntranceTrackerEnabled", 0);
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(600, 375), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Rastreador de entradas", &open, ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::End();
        return;
    }

    // Begin tracker settings
    ImGui::SetNextItemOpen(false, ImGuiCond_Once);
    if (ImGui::TreeNode("Ajustes del rastreador")) {
        // Reduce indentation from the tree node for the table
        ImGui::SetCursorPosX((ImGui::GetCursorPosX() / 2) + 4.0f);

        if (ImGui::BeginTable("entranceTrackerSettings", 1, ImGuiTableFlags_BordersInnerH)) {

            ImGui::TableNextColumn();

            UIWidgets::Spacer(0);
            ImGui::TextWrapped("El rastreador solo sigue las entradas aleatorizadas");
            UIWidgets::Spacer(0);

            ImGui::TableNextColumn();

            if (ImGui::BeginTable("entranceTrackerSubSettings", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp)) {

                ImGui::TableNextColumn();

                ImGui::Text("Ordenar por");
                UIWidgets::EnhancementRadioButton("Hacia", "gEntranceTrackerSortBy", 0);
                UIWidgets::Tooltip("Ordena las entradas por la entrada de origen original");
                UIWidgets::EnhancementRadioButton("Desde", "gEntranceTrackerSortBy", 1);
                UIWidgets::Tooltip("Ordena las entradas por el destino reemplazado");

                UIWidgets::Spacer(2.0f);

                ImGui::Text("Elementos de la lista");
                UIWidgets::PaddedEnhancementCheckbox("Desplazar solo", "gEntranceTrackerAutoScroll", true, false);
                UIWidgets::Tooltip("Se desplaza a la primera entrada disponible de la escena actual");
                UIWidgets::PaddedEnhancementCheckbox("Resaltar anterior", "gEntranceTrackerHighlightPrevious", true, false);
                UIWidgets::Tooltip("Resalta la entrada por la que llegó Link");
                UIWidgets::PaddedEnhancementCheckbox("Resaltar disponibles", "gEntranceTrackerHighlightAvailable", true, false);
                UIWidgets::Tooltip("Resalta las entradas disponibles de la escena actual");
                UIWidgets::PaddedEnhancementCheckbox("Ocultar sin descubrir", "gEntranceTrackerCollapseUndiscovered", true, false);
                UIWidgets::Tooltip("Manda las entradas sin descubrir al final de cada grupo");
                bool disableHideReverseEntrances = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_DECOUPLED_ENTRANCES) == RO_GENERIC_ON;
                static const char* disableHideReverseEntrancesText = "Opción desactivada porque \"Desacoplar entradas\" está activado.";
                UIWidgets::PaddedEnhancementCheckbox("Ocultar inversas", "gEntranceTrackerHideReverseEntrances", true, false,
                                              disableHideReverseEntrances, disableHideReverseEntrancesText, UIWidgets::CheckboxGraphics::Cross, true);
                UIWidgets::Tooltip("Oculta las transiciones inversas cuando Desacoplar entradas está desactivado");
                UIWidgets::Spacer(0);

                ImGui::TableNextColumn();

                ImGui::Text("Agrupar por");
                UIWidgets::EnhancementRadioButton("Zona", "gEntranceTrackerGroupBy", 0);
                UIWidgets::Tooltip("Agrupa las entradas por zona");
                UIWidgets::EnhancementRadioButton("Tipo", "gEntranceTrackerGroupBy", 1);
                UIWidgets::Tooltip("Agrupa las entradas por tipo");

                UIWidgets::Spacer(2.0f);

                ImGui::Text("Revelar spoiler");
                UIWidgets::PaddedEnhancementCheckbox("Mostrar \"Hacia\"", "gEntranceTrackerShowTo", true, false);
                UIWidgets::Tooltip("Revela la entrada \"Hacia\" de las entradas sin descubrir");
                UIWidgets::PaddedEnhancementCheckbox("Mostrar \"Desde\"", "gEntranceTrackerShowFrom", true, false);
                UIWidgets::Tooltip("Revela la entrada \"Desde\" de las entradas sin descubrir");

                ImGui::EndTable();
            }

            ImGui::TableNextColumn();

            ImGui::SetNextItemOpen(false, ImGuiCond_Once);
            if (ImGui::TreeNode("Leyenda")) {
                ImGui::TextColored(ImColor(COLOR_ORANGE), "Última entrada");
                ImGui::TextColored(ImColor(COLOR_GREEN), "Entradas disponibles");
                ImGui::TextColored(ImColor(COLOR_GRAY), "Entradas sin descubrir");
                ImGui::TreePop();
            }

            UIWidgets::Spacer(0);

            ImGui::EndTable();
        }

        ImGui::TreePop();
    } else {
        UIWidgets::PaddedSeparator();
    }

    static ImGuiTextFilter locationSearch;

    uint8_t nextTreeState = 0;
    if (ImGui::Button("Contraer todo")) {
        nextTreeState = 1;
    }
    UIWidgets::Tooltip("Contrae todos los grupos de entradas");
    ImGui::SameLine();
    if (ImGui::Button("Expandir todo")) {
        nextTreeState = 2;
    }
    UIWidgets::Tooltip("Expande todos los grupos de entradas");
    ImGui::SameLine();
    if (ImGui::Button("Borrar")) {
        locationSearch.Clear();
    }
    UIWidgets::Tooltip("Borra el campo de búsqueda");

    if (locationSearch.Draw()) {
        nextTreeState = 2;
    }

    uint8_t destToggle = CVarGetInteger("gEntranceTrackerSortBy", 0);
    uint8_t groupToggle = CVarGetInteger("gEntranceTrackerGroupBy", 0);

    // Combine destToggle and groupToggle to get a range of 0-3
    uint8_t groupType = destToggle + (groupToggle * 2);
    size_t groupCount = groupToggle ? ENTRANCE_TYPE_COUNT : SPOILER_ENTRANCE_GROUP_COUNT;
    auto groupNames = groupToggle ? groupTypeNames : spoilerEntranceGroupNames;

    EntranceOverride *entranceList;

    switch (groupType) {
        case ENTRANCE_SOURCE_AREA:
            entranceList = srcListSortedByArea;
            break;
        case ENTRANCE_DESTINATION_AREA:
            entranceList = destListSortedByArea;
            break;
        case ENTRANCE_SOURCE_TYPE:
            entranceList = srcListSortedByType;
            break;
        case ENTRANCE_DESTINATION_TYPE:
            entranceList = destListSortedByType;
            break;
    }

    // Begin tracker list
    ImGui::BeginChild("ChildEntranceTrackerLocations", ImVec2(0, -8));
    for (size_t i = 0; i < groupCount; i++) {
        std::string groupName = groupNames[i];

        uint16_t entranceCount = gEntranceTrackingData.GroupEntranceCounts[groupType][i];
        uint16_t startIndex = gEntranceTrackingData.GroupOffsets[groupType][i];

        bool doAreaScroll = false;
        size_t undiscovered = 0;
        std::vector<EntranceOverride> displayEntrances = {};

        // Loop over entrances first for filtering
        for (size_t entranceIdx = 0; entranceIdx < entranceCount; entranceIdx++) {
            size_t trueIdx = entranceIdx + startIndex;

            EntranceOverride entrance = entranceList[trueIdx];

            const EntranceData* original = GetEntranceData(entrance.index);
            const EntranceData* override = GetEntranceData(entrance.override);

            // If entrance is a dungeon, grotto, or interior entrance, the transition into that area has oneExit set, which means we can filter the return transitions as redundant
            // if entrances are not decoupled, as this is redundant information. Also checks a setting, enabled by default, for hiding them.
            // If all of these conditions are met, we skip adding this entrance to any lists.
            // However, if entrances are decoupled, then all transitions need to be displayed, so we proceed with the filtering
            if ((original->type == ENTRANCE_TYPE_DUNGEON || original->type == ENTRANCE_TYPE_GROTTO || original->type == ENTRANCE_TYPE_INTERIOR) &&
                (original->oneExit != 1 && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_DECOUPLED_ENTRANCES) == RO_GENERIC_OFF) &&
                CVarGetInteger("gEntranceTrackerHideReverseEntrances", 1) == 1) {
                    continue;
            }

            bool isDiscovered = IsEntranceDiscovered(entrance.index);

            bool showOriginal = (!destToggle ? CVarGetInteger("gEntranceTrackerShowTo", 0) : CVarGetInteger("gEntranceTrackerShowFrom", 0)) || isDiscovered;
            bool showOverride = (!destToggle ? CVarGetInteger("gEntranceTrackerShowFrom", 0) : CVarGetInteger("gEntranceTrackerShowTo", 0)) || isDiscovered;

            const char* origSrcAreaName = spoilerEntranceGroupNames[original->srcGroup].c_str();
            const char* origTypeName = groupTypeNames[original->type].c_str();
            const char* rplcSrcAreaName = spoilerEntranceGroupNames[override->srcGroup].c_str();
            const char* rplcTypeName = groupTypeNames[override->type].c_str();

            const char* origSrcName = showOriginal ? original->source.c_str()      : "";
            const char* origDstName = showOriginal ? original->destination.c_str() : "";
            const char* rplcSrcName = showOverride ? override->source.c_str()      : "";
            const char* rplcDstName = showOverride ? override->destination.c_str() : "";

            // Filter for entrances by group name, type, source/destination names, and meta tags
            if ((!locationSearch.IsActive() && (showOriginal || showOverride || !CVarGetInteger("gEntranceTrackerCollapseUndiscovered", 0))) ||
                ((showOriginal && (locationSearch.PassFilter(origSrcName) ||
                locationSearch.PassFilter(origDstName) || locationSearch.PassFilter(origSrcAreaName) ||
                locationSearch.PassFilter(origTypeName) || locationSearch.PassFilter(original->metaTag.c_str()))) ||
                (showOverride && (locationSearch.PassFilter(rplcSrcName) ||
                locationSearch.PassFilter(rplcDstName) || locationSearch.PassFilter(rplcSrcAreaName) ||
                locationSearch.PassFilter(rplcTypeName) || locationSearch.PassFilter(override->metaTag.c_str()))))) {

                // Detect if a scroll should happen and remember the scene for that scroll
                if (!doAreaScroll && (lastSceneOrEntranceDetected != LinkIsInArea(original) && LinkIsInArea(original) != -1)) {
                    lastSceneOrEntranceDetected = LinkIsInArea(original);
                    doAreaScroll = true;
                }

                displayEntrances.push_back(entrance);
            } else {
                if (!isDiscovered) {
                    undiscovered++;
                }
            }
        }

        // Then display the entrances in groups
        if (displayEntrances.size() != 0 || (!locationSearch.IsActive() && undiscovered > 0)) {
            // Handle opening/closing trees based on auto scroll or collapse/expand buttons
            if (nextTreeState == 1) {
                ImGui::SetNextItemOpen(false, ImGuiCond_None);
            } else {
                ImGui::SetNextItemOpen(true, nextTreeState == 0 && !doAreaScroll ? ImGuiCond_Once : ImGuiCond_None);
            }

            if (ImGui::TreeNode(groupName.c_str())) {
                for (auto entrance : displayEntrances) {
                    const EntranceData* original = GetEntranceData(entrance.index);
                    const EntranceData* override = GetEntranceData(entrance.override);

                    bool isDiscovered = IsEntranceDiscovered(entrance.index);

                    bool showOriginal = (!destToggle ? CVarGetInteger("gEntranceTrackerShowTo", 0) : CVarGetInteger("gEntranceTrackerShowFrom", 0)) || isDiscovered;
                    bool showOverride = (!destToggle ? CVarGetInteger("gEntranceTrackerShowFrom", 0) : CVarGetInteger("gEntranceTrackerShowTo", 0)) || isDiscovered;

                    const char* unknown = "???";

                    const char* origSrcName = showOriginal ? original->source.c_str()      : unknown;
                    const char* origDstName = showOriginal ? original->destination.c_str() : unknown;
                    const char* rplcSrcName = showOverride ? override->source.c_str()      : unknown;
                    const char* rplcDstName = showOverride ? override->destination.c_str() : unknown;

                    uint32_t color = isDiscovered ? IM_COL32_WHITE : COLOR_GRAY;

                    // Handle highlighting and auto scroll
                    if ((original->index == lastEntranceIndex ||
                        (override->reverseIndex == lastEntranceIndex && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_DECOUPLED_ENTRANCES) == RO_GENERIC_OFF)) &&
                            CVarGetInteger("gEntranceTrackerHighlightPrevious", 0)) {
                                 color = COLOR_ORANGE;
                    } else if (LinkIsInArea(original) != -1) {
                        if (CVarGetInteger("gEntranceTrackerHighlightAvailable", 0)) {
                            color = COLOR_GREEN;
                        }

                        if (doAreaScroll) {
                            doAreaScroll = false;
                            if (CVarGetInteger("gEntranceTrackerAutoScroll", 0)) {
                                ImGui::SetScrollHereY(0.0f);
                            }
                        }
                    }

                    ImGui::PushStyleColor(ImGuiCol_Text, color);

                    // Use a non-breaking space to keep the arrow from wrapping to a newline by itself
                    auto nbsp = u8"\u00A0";
                    if (original->srcGroup != ENTRANCE_GROUP_ONE_WAY) {
                        ImGui::TextWrapped("%s a %s%s->", origSrcName, origDstName, nbsp);
                    } else {
                        ImGui::TextWrapped("%s%s->", origSrcName, nbsp);
                    }

                    // Indent the destination
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() * 2);
                    if (!showOverride || (showOverride && (!override->oneExit && override->srcGroup != ENTRANCE_GROUP_ONE_WAY))) {
                        ImGui::TextWrapped("%s desde %s", rplcDstName, rplcSrcName);
                    } else {
                        ImGui::TextWrapped("%s", rplcDstName);
                    }

                    ImGui::PopStyleColor();
                }

                // Write collapsed undiscovered info
                if (!locationSearch.IsActive() && undiscovered > 0) {
                    UIWidgets::Spacer(0);
                    ImGui::PushStyleColor(ImGuiCol_Text, COLOR_GRAY);
                    ImGui::TextWrapped("%d sin descubrir", undiscovered);
                    ImGui::PopStyleColor();
                }

                UIWidgets::Spacer(0);
                ImGui::TreePop();
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void InitEntranceTracker() {
    SohImGui::AddWindow("Randomizer", "Entrance Tracker", DrawEntranceTracker, CVarGetInteger("gEntranceTrackerEnabled", 0) == 1);

    // Setup hooks for loading and clearing the entrance tracker data
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t fileNum) {
        InitEntranceTrackingData();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>([](int32_t fileNum) {
        ClearEntranceTrackingData();
    });
}
