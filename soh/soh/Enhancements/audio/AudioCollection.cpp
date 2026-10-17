#include "AudioCollection.h"
#include "sequence.h"
#include "sfx.h"
#include <vector>
#include <Utils/StringHelper.h>
#include <libultraship/bridge.h>
#include <libultraship/classes.h>
#include <locale>
#include <filesystem>

#define SEQUENCE_MAP_ENTRY(sequenceId, label, sfxKey, category) \
    { sequenceId, { sequenceId, label, sfxKey, category } }

AudioCollection::AudioCollection() {
    //                    (originalSequenceId,           label,                                 sfxKey,                          category),
    sequenceMap = {
        SEQUENCE_MAP_ENTRY(NA_BGM_FIELD_LOGIC,           "Llanura de Hyrule",                        "NA_BGM_FIELD_LOGIC",             SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_DUNGEON,               "Cueva de los dodongos",                    "NA_BGM_DUNGEON",                 SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_KAKARIKO_ADULT,        "Kakariko (adulto)",            "NA_BGM_KAKARIKO_ADULT",          SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_ENEMY,                 "Combate",                              "NA_BGM_ENEMY",                   SEQ_BGM_BATTLE),
        SEQUENCE_MAP_ENTRY(NA_BGM_BOSS,                  "Combate contra jefe",                         "NA_BGM_BOSS",                    SEQ_BGM_BATTLE),
        SEQUENCE_MAP_ENTRY(NA_BGM_INSIDE_DEKU_TREE,      "Interior del Gran Árbol Deku",                "NA_BGM_INSIDE_DEKU_TREE",        SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_MARKET,                "Plaza del mercado",                              "NA_BGM_MARKET",                  SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_TITLE,                 "Tema del título",                         "NA_BGM_TITLE",                   SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_LINK_HOUSE,            "Casa",                               "NA_BGM_LINK_HOUSE",              SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_GAME_OVER,             "Fin de la partida",                           "NA_BGM_GAME_OVER",               SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_BOSS_CLEAR,            "Jefe derrotado",                          "NA_BGM_BOSS_CLEAR",              SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_ITEM_GET,              "Obtener objeto",                         "NA_BGM_ITEM_GET",                SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_OPENING_GANON,         "Entra Ganondorf",                     "NA_BGM_OPENING_GANON",           SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_HEART_GET,             "Obtener contenedor de corazón",              "NA_BGM_HEART_GET",               SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_LIGHT,             "Preludio de la luz",                    "NA_BGM_OCA_LIGHT",               SEQ_OCARINA),
        SEQUENCE_MAP_ENTRY(NA_BGM_JABU_JABU,             "Interior de la tripa de Jabu-Jabu",            "NA_BGM_JABU_JABU",               SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_KAKARIKO_KID,          "Kakariko (niño)",            "NA_BGM_KAKARIKO_KID",            SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_GREAT_FAIRY,           "Fuente de la Gran hada",              "NA_BGM_GREAT_FAIRY",             SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_ZELDA_THEME,           "Tema de Zelda",                       "NA_BGM_ZELDA_THEME",             SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_FIRE_TEMPLE,           "Templo del Fuego",                         "NA_BGM_FIRE_TEMPLE",             SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_OPEN_TRE_BOX,          "Abrir cofre del tesoro",                 "NA_BGM_OPEN_TRE_BOX",            SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_FOREST_TEMPLE,         "Templo del Bosque",                       "NA_BGM_FOREST_TEMPLE",           SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_COURTYARD,             "Jardín del castillo de Hyrule",             "NA_BGM_COURTYARD",               SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_GANON_TOWER,           "Tema de Ganondorf",                   "NA_BGM_GANON_TOWER",             SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_LONLON,                "Rancho Lon Lon",                       "NA_BGM_LONLON",                  SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_GORON_CITY,            "Ciudad Goron",                          "NA_BGM_GORON_CITY",              SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_FIELD_MORNING,         "Llanura de Hyrule (mañana)",          "NA_BGM_FIELD_MORNING",           SEQ_NOSHUFFLE),
        SEQUENCE_MAP_ENTRY(NA_BGM_SPIRITUAL_STONE,       "Obtener piedra espiritual",                 "NA_BGM_SPIRITUAL_STONE",         SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_BOLERO,            "Bolero del fuego",                      "NA_BGM_OCA_BOLERO",              SEQ_OCARINA),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_MINUET,            "Minueto del bosque",                    "NA_BGM_OCA_MINUET",              SEQ_OCARINA),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_SERENADE,          "Serenata del agua",                   "NA_BGM_OCA_SERENADE",            SEQ_OCARINA),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_REQUIEM,           "Réquiem del espíritu",                   "NA_BGM_OCA_REQUIEM",             SEQ_OCARINA),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_NOCTURNE,          "Nocturno de la sombra",                  "NA_BGM_OCA_NOCTURNE",            SEQ_OCARINA),
        SEQUENCE_MAP_ENTRY(NA_BGM_MINI_BOSS,             "Combate contra minijefe",                    "NA_BGM_MINI_BOSS",               SEQ_BGM_BATTLE),
        SEQUENCE_MAP_ENTRY(NA_BGM_SMALL_ITEM_GET,        "Obtener objeto pequeño",                   "NA_BGM_SMALL_ITEM_GET",          SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_TEMPLE_OF_TIME,        "Templo del Tiempo",                      "NA_BGM_TEMPLE_OF_TIME",          SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_EVENT_CLEAR,           "Huida del Rancho Lon Lon",           "NA_BGM_EVENT_CLEAR",             SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_KOKIRI,                "Bosque Kokiri",                       "NA_BGM_KOKIRI",                  SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_FAIRY_GET,         "Obtener Ocarina de las Hadas",                "NA_BGM_OCA_FAIRY_GET",           SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_SARIA_THEME,           "Bosque Perdido",                          "NA_BGM_SARIA_THEME",             SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_SPIRIT_TEMPLE,         "Templo del Espíritu",                       "NA_BGM_SPIRIT_TEMPLE",           SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_HORSE,                 "Carrera de caballos",                          "NA_BGM_HORSE",                   SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_HORSE_GOAL,            "Meta de la carrera de caballos",                     "NA_BGM_HORSE_GOAL",              SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_INGO,                  "Tema de Ingo",                        "NA_BGM_INGO",                    SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_MEDALLION_GET,         "Obtener medallón",                    "NA_BGM_MEDALLION_GET",           SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_SARIA,             "Ocarina: Canción de Saria",                "NA_BGM_OCA_SARIA",               SEQ_OCARINA),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_EPONA,             "Ocarina: Canción de Epona",                "NA_BGM_OCA_EPONA",               SEQ_OCARINA),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_ZELDA,             "Ocarina: Nana de Zelda",             "NA_BGM_OCA_ZELDA",               SEQ_OCARINA),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_SUNS,              "Ocarina: Canción del Sol",                  "NA_BGM_OCA_SUNS",                SEQ_OCARINA),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_TIME,              "Ocarina: Canción del tiempo",                "NA_BGM_OCA_TIME",                SEQ_OCARINA),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCA_STORM,             "Ocarina: Canción de la tormenta",              "NA_BGM_OCA_STORM",               SEQ_OCARINA),
        SEQUENCE_MAP_ENTRY(NA_BGM_NAVI_OPENING,          "Vuelo del hada",                        "NA_BGM_NAVI_OPENING",            SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_DEKU_TREE_CS,          "Gran Árbol Deku",                           "NA_BGM_DEKU_TREE_CS",            SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_WINDMILL,              "Cabaña del molino",                        "NA_BGM_WINDMILL",                SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_HYRULE_CS,             "Leyenda de Hyrule",                    "NA_BGM_HYRULE_CS",               SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_MINI_GAME,             "Galería de tiro",                    "NA_BGM_MINI_GAME",               SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_SHEIK,                 "Tema de Sheik",                       "NA_BGM_SHEIK",                   SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_ZORA_DOMAIN,           "Región de los zora",                       "NA_BGM_ZORA_DOMAIN",             SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_APPEAR,                "Entra Zelda",                         "NA_BGM_APPEAR",                  SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_ADULT_LINK,            "Adiós a Zelda",                    "NA_BGM_ADULT_LINK",              SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_MASTER_SWORD,          "Espada Maestra",                        "NA_BGM_MASTER_SWORD",            SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_INTRO_GANON,           "Introducción de Ganon",                         "NA_BGM_INTRO_GANON",             SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_SHOP,                  "Tienda",                                "NA_BGM_SHOP",                    SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_CHAMBER_OF_SAGES,      "Sala de los Sabios",                "NA_BGM_CHAMBER_OF_SAGES",        SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_FILE_SELECT,           "Selección de archivo",                         "NA_BGM_FILE_SELECT",             SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_ICE_CAVERN,            "Caverna de hielo",                          "NA_BGM_ICE_CAVERN",              SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_DOOR_OF_TIME,          "Abrir puerta del Templo del Tiempo",         "NA_BGM_DOOR_OF_TIME",            SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_OWL,                   "Tema de Kaepora Gaebora",             "NA_BGM_OWL",                     SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_SHADOW_TEMPLE,         "Templo de las Sombras",                       "NA_BGM_SHADOW_TEMPLE",           SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_WATER_TEMPLE,          "Templo del Agua",                        "NA_BGM_WATER_TEMPLE",            SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_BRIDGE_TO_GANONS,      "Puente al castillo de Ganon",               "NA_BGM_BRIDGE_TO_GANONS",        SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_OCARINA_OF_TIME,       "Ocarina del Tiempo",                     "NA_BGM_OCARINA_OF_TIME",         SEQ_FANFARE),
        SEQUENCE_MAP_ENTRY(NA_BGM_GERUDO_VALLEY,         "Valle Gerudo",                       "NA_BGM_GERUDO_VALLEY",           SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_POTION_SHOP,           "Tienda de pociones",                         "NA_BGM_POTION_SHOP",             SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_KOTAKE_KOUME,          "Tema de Kotake y Koume",              "NA_BGM_KOTAKE_KOUME",            SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_ESCAPE,                "Huida del castillo de Ganon",          "NA_BGM_ESCAPE",                  SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(NA_BGM_UNDERGROUND,           "Subsuelo del castillo de Ganon",         "NA_BGM_UNDERGROUND",             SEQ_BGM_WORLD),
        SEQUENCE_MAP_ENTRY(NA_BGM_GANONDORF_BOSS,        "Combate contra Ganondorf",                    "NA_BGM_GANONDORF_BOSS",          SEQ_BGM_BATTLE),
        SEQUENCE_MAP_ENTRY(NA_BGM_GANON_BOSS,            "Combate contra Ganon",                        "NA_BGM_GANON_BOSS",              SEQ_BGM_BATTLE),
        SEQUENCE_MAP_ENTRY(NA_BGM_END_DEMO,              "Sello de los seis sabios",                   "NA_BGM_END_DEMO",                SEQ_NOSHUFFLE),
        SEQUENCE_MAP_ENTRY(NA_BGM_STAFF_1,               "Créditos finales I",                       "NA_BGM_STAFF_1",                 SEQ_NOSHUFFLE),
        SEQUENCE_MAP_ENTRY(NA_BGM_STAFF_2,               "Créditos finales II",                      "NA_BGM_STAFF_2",                 SEQ_NOSHUFFLE),
        SEQUENCE_MAP_ENTRY(NA_BGM_STAFF_3,               "Créditos finales III",                     "NA_BGM_STAFF_3",                 SEQ_NOSHUFFLE),
        SEQUENCE_MAP_ENTRY(NA_BGM_STAFF_4,               "Créditos finales IV",                      "NA_BGM_STAFF_4",                 SEQ_NOSHUFFLE),
        SEQUENCE_MAP_ENTRY(NA_BGM_FIRE_BOSS,             "Jefe: Rey Dodongo y Volvagia", "NA_BGM_FIRE_BOSS",               SEQ_BGM_BATTLE),
        SEQUENCE_MAP_ENTRY(NA_BGM_TIMED_MINI_GAME,       "Minijuego",                           "NA_BGM_TIMED_MINI_GAME",         SEQ_BGM_EVENT),
        SEQUENCE_MAP_ENTRY(INSTRUMENT_OFFSET + 1,        "Ocarina",                             "OCARINA_INSTRUMENT_DEFAULT",     SEQ_INSTRUMENT),
        SEQUENCE_MAP_ENTRY(INSTRUMENT_OFFSET + 2,        "Malon",                               "OCARINA_INSTRUMENT_MALON",       SEQ_INSTRUMENT),
        SEQUENCE_MAP_ENTRY(INSTRUMENT_OFFSET + 3,        "Silbato",                             "OCARINA_INSTRUMENT_WHISTLE",     SEQ_INSTRUMENT),
        SEQUENCE_MAP_ENTRY(INSTRUMENT_OFFSET + 4,        "Arpa",                                "OCARINA_INSTRUMENT_HARP",        SEQ_INSTRUMENT),
        SEQUENCE_MAP_ENTRY(INSTRUMENT_OFFSET + 5,        "Órgano",                               "OCARINA_INSTRUMENT_GRIND_ORGAN", SEQ_INSTRUMENT),
        SEQUENCE_MAP_ENTRY(INSTRUMENT_OFFSET + 6,        "Flauta",                               "OCARINA_INSTRUMENT_FLUTE",       SEQ_INSTRUMENT),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_SMALL_DOG_BARK,      "Ladrido",                                "NA_SE_EV_SMALL_DOG_BARK",        SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_AWA_BOUND,           "Rebote de bomba",                         "NA_SE_EN_AWA_BOUND",             SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_SHADEST_TAIKO_LOW,   "Bongo Bongo grave",                     "NA_SE_EN_SHADEST_TAIKO_LOW",     SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_NUTS_FAINT,          "Deku mercader",                      "NA_SE_EN_NUTS_FAINT",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_SY_CARROT_RECOVER,      "Recarga de zanahoria",                       "NA_SE_SY_CARROT_RECOVER",        SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_CHICKEN_CRY_N,       "Cacareo",                               "NA_SE_EV_CHICKEN_CRY_N",         SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_BRIDGE_OPEN_STOP,    "Puente levadizo",                      "NA_SE_EV_BRIDGE_OPEN_STOP",      SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_KAICHO_CRY,          "Guay",                                "NA_SE_EN_KAICHO_CRY",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_SY_HITPOINT_ALARM,      "Pitido de poca vida",                         "NA_SE_SY_HITPOINT_ALARM",        SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_SY_HP_RECOVER,          "Recuperar vida",                          "NA_SE_SY_HP_RECOVER",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_HORSE_RUN,           "Trote de caballo",                          "NA_SE_EV_HORSE_RUN",             SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_PL_WALK_HEAVYBOOTS,     "Botas de hierro",                          "NA_SE_PL_WALK_HEAVYBOOTS",       SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_COW_CRY,             "Mu",                                 "NA_SE_EV_COW_CRY",               SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_KZ_MOVE,             "¡Mweep!",                              "NA_SE_VO_KZ_MOVE",               SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_NA_HELLO_2,          "Navi: ¡Ey!",                           "NA_SE_VO_NA_HELLO_2",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_SK_LAUGH,            "Navi: ¡Escucha!",                        "NA_SE_VO_SK_LAUGH",              SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_POT_BROKEN,          "Vasija rota",                      "NA_SE_EV_POT_BROKEN",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_FROG_CRY_0,          "Croac",                              "NA_SE_EV_FROG_CRY_0",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_FIVE_COUNT_LUPY,     "Rupia (plata)",                      "NA_SE_EV_FIVE_COUNT_LUPY",       SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_FOOT_SWITCH,         "Interruptor",                              "NA_SE_EV_FOOT_SWITCH",           SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_IT_WALL_HIT_SOFT,       "Choque de espada",                          "NA_SE_IT_WALL_HIT_SOFT",         SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_SY_METRONOME,           "Pandereta",                          "NA_SE_SY_METRONOME",             SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_Z1_SURPRISE,         "Jadeo de Zelda (adulta)",                  "NA_SE_VO_Z1_SURPRISE",           SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_AMOS_VOICE,          "Armos",                               "NA_SE_EN_AMOS_VOICE",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_LI_FALL_L_KID,       "Grito de niño",                        "NA_SE_VO_LI_FALL_L_KID",         SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_FR_LAUGH_0,          "Gran hada",                         "NA_SE_VO_FR_LAUGH_0",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_REDEAD_AIM,          "Grito de Redead",                       "NA_SE_EN_REDEAD_AIM",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_RT_LAUGH_0,          "Risita de Ruto",                         "NA_SE_VO_RT_LAUGH_0",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_STALKID_ATTACK,      "Ataque de Stalchild",                    "NA_SE_EN_STALKID_ATTACK",        SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_CHICKEN_CRY_M,       "Quiquiriquí",                      "NA_SE_EV_CHICKEN_CRY_M",         SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_SY_KINSTA_MARK_APPEAR,  "Símbolo de skulltula dorada",                "NA_SE_SY_KINSTA_MARK_APPEAR",    SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_REDEAD_CRY,          "Gemido de Redead",                         "NA_SE_EN_REDEAD_CRY",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_TA_SLEEP,            "Ronquido de Talon",                         "NA_SE_VO_TA_SLEEP",              SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_LIGHTNING,           "Trueno",                             "NA_SE_EV_LIGHTNING",             SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_OC_ABYSS,               "Caída de dibujos",                        "NA_SE_OC_ABYSS",                 SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_FLAME_LAUGH,         "Risa del bailarín de fuego",                  "NA_SE_EN_FLAME_LAUGH",           SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_AWA_BREAK,           "Pop de Shabom",                          "NA_SE_EN_AWA_BREAK",             SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_SHADEST_TAIKO_HIGH,  "Bongo Bongo agudo",                    "NA_SE_EN_SHADEST_TAIKO_HIGH",    SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_BOTTLE_CAP_OPEN,     "Tapón de botella",                         "NA_SE_EV_BOTTLE_CAP_OPEN",       SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_IT_BOW_FLICK,           "Disparo de arco",                           "NA_SE_IT_BOW_FLICK",             SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_BUBLE_LAUGH,         "Risa de burbuja",                        "NA_SE_EN_BUBLE_LAUGH",           SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_LI_BREATH_DRINK_KID, "Jadeo de niño",                          "NA_SE_VO_LI_BREATH_DRINK_KID",   SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_DEKU_JR_MOUTH,       "Deku Baba",                           "NA_SE_EN_DEKU_JR_MOUTH",         SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_DOG_CRY_EVENING,     "Aullido del anochecer",                           "NA_SE_EV_DOG_CRY_EVENING",       SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_FLAME_DAMAGE,        "Bailarín de fuego sobresaltado",               "NA_SE_EN_FLAME_DAMAGE",          SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_GANON_AT_RETURN,     "¡Teh! de Ganondorf",                      "NA_SE_EN_GANON_AT_RETURN",       SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_GOMA_JR_CRY,         "Croar de larva de Gohma",                   "NA_SE_EN_GOMA_JR_CRY",           SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_GOLON_WAKE_UP,       "Despertar de Goron",                          "NA_SE_EN_GOLON_WAKE_UP",         SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_SY_START_SHOT,          "Disparo",                             "NA_SE_SY_START_SHOT",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_IT_HAMMER_HIT,          "Golpe de martillo",                         "NA_SE_IT_HAMMER_HIT",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_IRONNACK_SWING_AXE,  "Iron Knuckle",                        "NA_SE_EN_IRONNACK_SWING_AXE",    SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_FANTOM_ST_LAUGH,     "Risa de Ganon fantasma",                 "NA_SE_EN_FANTOM_ST_LAUGH",       SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_PLANT_BROKEN,        "Planta que explota",                       "NA_SE_EV_PLANT_BROKEN",          SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_SY_GET_RUPY,            "Rupia",                               "NA_SE_SY_GET_RUPY",              SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_RT_CRASH,            "Choque de Ruto",                          "NA_SE_VO_RT_CRASH",              SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_RT_LIFT,             "Ruto alzada",                           "NA_SE_VO_RT_LIFT",               SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_RT_THROW,            "Ruto lanzada",                         "NA_SE_VO_RT_THROW",              SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_NUTS_UP,             "Deku saliendo",                        "NA_SE_EN_NUTS_UP",               SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_AWA_BOUND,           "Rebote de Shabom",                       "NA_SE_EN_AWA_BOUND",             SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_SHELL_MOUTH,         "Shellblade",                          "NA_SE_EN_SHELL_MOUTH",           SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_STALTU_DAMAGE,       "Daño de Skulltula",                    "NA_SE_EN_STALTU_DAMAGE",         SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EN_NUTS_THROW,          "Escupir nuez",                            "NA_SE_EN_NUTS_THROW",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_TA_CRY_0,            "Talon: Hmm",                           "NA_SE_VO_TA_CRY_0",              SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_VO_TA_SURPRISE,         "Talon sorprendido",                     "NA_SE_VO_TA_SURPRISE",           SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_SY_LOCK_ON,             "Fijar enemigo",                        "NA_SE_SY_LOCK_ON",               SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_SY_LOCK_ON_HUMAN,       "Fijar neutral",                      "NA_SE_SY_LOCK_ON_HUMAN",         SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_SY_FSEL_CURSOR,         "Cursor de selección",                  "NA_SE_SY_FSEL_CURSOR",           SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_SY_FSEL_DECIDE_L,       "Elegir en selección",                  "NA_SE_SY_FSEL_DECIDE_L",         SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_SY_FSEL_CLOSE,          "Volver en selección",                    "NA_SE_SY_FSEL_CLOSE",            SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_IT_BOMB_EXPLOSION,      "Explosión de bomba",                      "NA_SE_IT_BOMB_EXPLOSION",        SEQ_SFX),
        SEQUENCE_MAP_ENTRY(NA_SE_EV_CHICKEN_CRY_A,       "Grito de gallina",                         "NA_SE_EV_CHICKEN_CRY_A",         SEQ_SFX),
    };

}

void AudioCollection::AddToCollection(char* otrPath, uint16_t seqNum) {
    std::string fileName = std::filesystem::path(otrPath).filename().string();
    std::vector<std::string> splitFileName = StringHelper::Split(fileName, "_");
    std::string sequenceName = splitFileName[0];
    SeqType type = SEQ_BGM_CUSTOM;
    std::string typeString = splitFileName[splitFileName.size() - 1];
    std::locale loc;
    for (size_t i = 0; i < typeString.length(); i++) {
        typeString[i] = std::tolower(typeString[i], loc);
    }
    if (typeString == "fanfare") {
        type = SEQ_FANFARE;
    }
    SequenceInfo info = {seqNum,
                         sequenceName,
                         StringHelper::Replace(StringHelper::Replace(StringHelper::Replace(sequenceName, " ", "_"), "~", "-"),".", ""),
                         type};
    sequenceMap.emplace(seqNum, info);
}

uint16_t AudioCollection::GetReplacementSequence(uint16_t seqId) {
    // if Hyrule Field Morning is about to play, but Hyrule Field is swapped, get the replacement sequence
    // for Hyrule Field instead. Otherwise, leave it alone, so that without any sfx editor modifications we will
    // play the normal track as usual.
    if (seqId == NA_BGM_FIELD_MORNING) {
        if (CVarGetInteger("gAudioEditor.ReplacedSequences.NA_BGM_FIELD_LOGIC", NA_BGM_FIELD_LOGIC) != NA_BGM_FIELD_LOGIC) {
            seqId = NA_BGM_FIELD_LOGIC;
        }
    }

    if (sequenceMap.find(seqId) == sequenceMap.end()) {
        return seqId;
    }

    const auto& sequenceInfo = sequenceMap.at(seqId);
    const std::string cvarKey = "gAudioEditor.ReplacedSequences." + sequenceInfo.sfxKey;
    int replacementSeq = CVarGetInteger(cvarKey.c_str(), seqId);
    if (!sequenceMap.contains(replacementSeq)) {
        replacementSeq = seqId;
    }
    return static_cast<uint16_t>(replacementSeq);
}

void AudioCollection::RemoveFromShufflePool(SequenceInfo* seqInfo) {
    const std::string cvarKey = "gAudioEditor.Excluded." + seqInfo->sfxKey;
    excludedSequences.insert(seqInfo);
    includedSequences.erase(seqInfo);
    CVarSetInteger(cvarKey.c_str(), 1);
    SohImGui::RequestCvarSaveOnNextTick();
}

void AudioCollection::AddToShufflePool(SequenceInfo* seqInfo) {
    const std::string cvarKey = "gAudioEditor.Excluded." + seqInfo->sfxKey;
    includedSequences.insert(seqInfo);
    excludedSequences.erase(seqInfo);
    CVarClear(cvarKey.c_str());
    SohImGui::RequestCvarSaveOnNextTick();
}

void AudioCollection::InitializeShufflePool() {
    if (shufflePoolInitialized) return;
    
    for (auto& [seqId, seqInfo] : sequenceMap) {
        const std::string cvarKey = "gAudioEditor.Excluded." + seqInfo.sfxKey;
        if (CVarGetInteger(cvarKey.c_str(), 0)) {
            excludedSequences.insert(&seqInfo);
        } else {
            if (seqInfo.category != SEQ_NOSHUFFLE) {
                includedSequences.insert(&seqInfo);
            }
        }
    }

    shufflePoolInitialized = true;
};

extern "C" void AudioCollection_AddToCollection(char *otrPath, uint16_t seqNum) {
    AudioCollection::Instance->AddToCollection(otrPath, seqNum);
}

const char* AudioCollection::GetSequenceName(uint16_t seqId) {
    auto seqIt = sequenceMap.find(seqId);
    if (seqIt != sequenceMap.end()) {
        return seqIt->second.label.c_str();
    }
    return nullptr;
}

extern "C" const char* AudioCollection_GetSequenceName(uint16_t seqId) {
    return AudioCollection::Instance->GetSequenceName(seqId);
}