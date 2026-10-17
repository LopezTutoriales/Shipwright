#include "actorViewer.h"
#include "../../util.h"
#include "../../UIWidgets.hpp"
#include <ImGuiImpl.h>

#include <array>
#include <bit>
#include <map>
#include <string>
#include <libultraship/bridge.h>

extern "C" {
#include <z64.h>
#include "z64math.h"
#include "variables.h"
#include "functions.h"
#include "macros.h"
extern PlayState* gPlayState;

#include "textures/icon_item_static/icon_item_static.h"
#include "textures/icon_item_24_static/icon_item_24_static.h"
}

typedef struct {
    u16 id;
    u16 params;
    Vec3f pos;
    Vec3s rot;
} ActorInfo;

typedef enum {
    LIST,
    TARGET,
    HELD,
    INTERACT
} RetrievalMethod;

std::array<const char*, 12> acMapping = { 
    "Interruptor",
    "Fondo (objeto tipo 1)",
    "Jugador",
    "Bomba",
    "PNJ",
    "Enemigo",
    "Objeto tipo 2",
    "Objeto/Acción",
    "Varios",
    "Jefe",
    "Puerta",
    "Cofre"
};

// https://wiki.cloudmodding.com/oot/Actor_List_(Variables)
std::map<u16, const char*> actorDescriptions = {
    { ACTOR_PLAYER, "Link" },
    { ACTOR_EN_TEST, "Stalfos" },
    { ACTOR_EN_GIRLA, "Artículos de tienda" },
    { ACTOR_EN_PART, "Partes del cuerpo" },
    { ACTOR_EN_LIGHT, "Llamas decorativas" },
    { ACTOR_EN_DOOR, "Puerta" },
    { ACTOR_EN_BOX, "Cofre del tesoro" },
    { ACTOR_BG_DY_YOSEIZO, "Gran hada" },
    { ACTOR_BG_HIDAN_FIREWALL, "Muro de fuego por proximidad" },
    { ACTOR_EN_POH, "Poe" },
    { ACTOR_EN_OKUTA, "Octorok" },
    { ACTOR_BG_YDAN_SP, "Telarañas" },
    { ACTOR_EN_BOM, "Bomba" },
    { ACTOR_EN_WALLMAS, "Wallmaster" },
    { ACTOR_EN_DODONGO, "Dodongo" },
    { ACTOR_EN_FIREFLY, "Keese" },
    { ACTOR_EN_HORSE, "Caballo montable" },
    { ACTOR_EN_ITEM00, "Coleccionables" },
    { ACTOR_EN_ARROW, "Flecha" },
    { ACTOR_EN_ELF, "Hada" },
    { ACTOR_EN_NIW, "Cuco" },
    { ACTOR_EN_TITE, "Tektite" },
    { ACTOR_EN_REEBA, "Leever" },
    { ACTOR_EN_PEEHAT, "Peahat y larva" },
    { ACTOR_EN_BUTTE, "Mariposa" },
    { ACTOR_EN_INSECT, "Bichos" },
    { ACTOR_EN_FISH, "Pez" },
    { ACTOR_EN_HOLL, "Plano de cambio de sala" },
    { ACTOR_EN_SCENE_CHANGE, "ovl_En_Scene_Change (roto)" },
    { ACTOR_EN_ZF, "Lizalfos y Dinolfos" },
    { ACTOR_EN_HATA, "Mástil de madera con tela roja" },
    { ACTOR_BOSS_DODONGO, "Rey Dodongo" },
    { ACTOR_BOSS_GOMA, "Gohma" },
    { ACTOR_EN_ZL1, "Princesa Zelda niña (en la ventana)" },
    { ACTOR_EN_VIEWER, "Actores de escenas" },
    { ACTOR_EN_GOMA, "Larva de Gohma" },
    { ACTOR_BG_PUSHBOX, "Cubo" },
    { ACTOR_EN_BUBBLE, "Shabom" },
    { ACTOR_DOOR_SHUTTER, "Puerta corredera" },
    { ACTOR_EN_DODOJR, "Dodongo bebé" },
    { ACTOR_EN_BDFIRE, "Vacío" },
    { ACTOR_EN_BOOM, "Bumerán" },
    { ACTOR_EN_TORCH2, "Link oscuro" },
    { ACTOR_EN_BILI, "Biri" },
    { ACTOR_EN_TP, "Tail Parasan eléctrico" },
    { ACTOR_EN_ST, "Skulltula" },
    { ACTOR_EN_BW, "Babosa de antorcha" },
    { ACTOR_EN_A_OBJ, "Objetos de Play_keep" },
    { ACTOR_EN_EIYER, "Stinger (tierra)" },
    { ACTOR_EN_RIVER_SOUND, "Efectos de sonido ambiente" },
    { ACTOR_EN_HORSE_NORMAL, "Caballo" },
    { ACTOR_EN_OSSAN, "Tendero" },
    { ACTOR_BG_TREEMOUTH, "Mandíbula del Gran Árbol Deku" },
    { ACTOR_BG_DODOAGO, "Mega Dodongo de la Cueva de los dodongos" },
    { ACTOR_BG_HIDAN_DALM, "Estatua del Martillo Megatón" },
    { ACTOR_BG_HIDAN_HROCK, "Enorme plataforma de pinchos de piedra (Templo del Fuego)" },
    { ACTOR_EN_HORSE_GANON, "Caballo de Ganondorf" },
    { ACTOR_BG_HIDAN_ROCK, "Bloques de piedra (Templo del Fuego)" },
    { ACTOR_BG_HIDAN_RSEKIZOU, "Lanzallamas de piedra giratorio (Templo del Fuego)" },
    { ACTOR_BG_HIDAN_SEKIZOU, "Estatua lanzallamas fija (Templo del Fuego)" },
    { ACTOR_BG_HIDAN_SIMA, "Plataforma de piedra (Templo del Fuego)" },
    { ACTOR_BG_HIDAN_SYOKU, "Ascensor de piedra (Templo del Fuego)" },
    { ACTOR_EN_XC, "Sheik" },
    { ACTOR_BG_HIDAN_CURTAIN, "Círculo de llamas" },
    { ACTOR_BG_SPOT00_HANEBASI, "Objetos del puente levadizo" },
    { ACTOR_EN_MB, "Moblins" },
    { ACTOR_EN_BOMBF, "Flor bomba" },
    { ACTOR_EN_ZL2, "Zelda adulta (escenas)" },
    { ACTOR_BG_HIDAN_FSLIFT, "Plataforma elevadora del gancho (Templo del Fuego)" },
    { ACTOR_EN_OE2, "PNJ sin usar" },
    { ACTOR_BG_YDAN_HASI, "Elementos de puzle del Árbol Deku" },
    { ACTOR_BG_YDAN_MARUTA, "Tronco con pinchos giratorio (Árbol Deku)" },
    { ACTOR_BOSS_GANONDROF, "Ganon fantasma" },
    { ACTOR_EN_AM, "Estatua Armos" },
    { ACTOR_EN_DEKUBABA, "Deku Baba" },
    { ACTOR_EN_M_FIRE1, "Nuez deku lanzada" },
    { ACTOR_EN_M_THUNDER, "Ataque giratorio" },
    { ACTOR_BG_DDAN_JD, "Plataforma de piedra ascendente (Cueva de los dodongos)" },
    { ACTOR_BG_BREAKWALL, "Muro destruible con bombas" },
    { ACTOR_EN_JJ, "Lord Jabu-Jabu" },
    { ACTOR_EN_HORSE_ZELDA, "Caballo de Zelda" },
    { ACTOR_BG_DDAN_KD, "Escaleras de piedra (Cueva de los dodongos)" },
    { ACTOR_DOOR_WARP1, "Portal de teletransporte" },
    { ACTOR_OBJ_SYOKUDAI, "Antorcha" },
    { ACTOR_ITEM_B_HEART, "Contenedor de corazón" },
    { ACTOR_EN_DEKUNUTS, "Mercader deku furioso" },
    { ACTOR_BG_MENKURI_KAITEN, "Plataforma de anillo de piedra giratoria" },
    { ACTOR_BG_MENKURI_EYE, "Estatua del ojo" },
    { ACTOR_EN_VALI, "Bari" },
    { ACTOR_BG_MIZU_MOVEBG, "Actores de fondo del Templo del Agua" },
    { ACTOR_BG_MIZU_WATER, "Plano de agua (Templo del Agua)" },
    { ACTOR_ARMS_HOOK, "Hookshot" },
    { ACTOR_EN_FHG, "Caballo de Ganon fantasma" },
    { ACTOR_BG_MORI_HINERI, "Salas del pasillo retorcido" },
    { ACTOR_EN_BB, "Burbuja" },
    { ACTOR_BG_TOKI_HIKARI, "Ventanas (Templo del Tiempo)" },
    { ACTOR_EN_YUKABYUN, "Baldosa voladora" },
    { ACTOR_BG_TOKI_SWD, "Master Sword" },
    { ACTOR_EN_FHG_FIRE, "Vacío" },
    { ACTOR_BG_MJIN, "Pedestal de canción de teletransporte" },
    { ACTOR_BG_HIDAN_KOUSI, "Verja metálica corredera" },
    { ACTOR_DOOR_TOKI, "Colisión de la Puerta del Tiempo" },
    { ACTOR_BG_HIDAN_HAMSTEP, "Escalones y plataformas de piedra (Templo del Fuego)" },
    { ACTOR_EN_BIRD, "Pájaro marrón" },
    { ACTOR_EN_WOOD02, "Árboles y arbustos" },
    { ACTOR_EN_LIGHTBOX, "Piedra grande y ruidosa" },
    { ACTOR_EN_PU_BOX, "Cubo de piedra" },
    { ACTOR_EN_TRAP, "Trampa de pinchos metálicos" },
    { ACTOR_EN_AROW_TRAP, "Trampa de flechas" },
    { ACTOR_EN_VASE, "Vasija naranja" },
    { ACTOR_EN_TA, "Talon" },
    { ACTOR_EN_TK, "Dampé (vivo)" },
    { ACTOR_BG_MORI_BIGST, "Gran plataforma redonda (Templo del Bosque)" },
    { ACTOR_BG_MORI_ELEVATOR, "Ascensor (Templo del Bosque)" },
    { ACTOR_BG_MORI_KAITENKABE, "Muros giratorios (Templo del Bosque)" },
    { ACTOR_BG_MORI_RAKKATENJO, "Techo que cae (Templo del Bosque)" },
    { ACTOR_EN_VM, "Beamos" },
    { ACTOR_DEMO_EFFECT, "Piedras espirituales, medallones, Trifuerza" },
    { ACTOR_DEMO_KANKYO, "Objetos del Templo del Tiempo" },
    { ACTOR_BG_HIDAN_FWBIG, "Gran muro de fuego (Templo del Fuego)" },
    { ACTOR_EN_FLOORMAS, "Floormaster" },
    { ACTOR_EN_HEISHI1, "Guardias del patio del castillo" },
    { ACTOR_EN_RD, "Redead y Gibdo" },
    { ACTOR_EN_PO_SISTERS, "Hermanas Poe" },
    { ACTOR_BG_HEAVY_BLOCK, "Pilar de los Guantes de Oro" },
    { ACTOR_BG_PO_EVENT, "Cuadros de las Hermanas Poe y bloque de puzle" },
    { ACTOR_OBJ_MURE, "Generador de peces, bichos y mariposas" },
    { ACTOR_EN_SW, "Skullwalltula y Skulltulas doradas" },
    { ACTOR_BOSS_FD, "Volvagia (volando)" },
    { ACTOR_OBJECT_KANKYO, "Efectos ambientales" },
    { ACTOR_EN_DU, "Darunia" },
    { ACTOR_EN_FD, "Flare Dancer" },
    { ACTOR_EN_HORSE_LINK_CHILD, "Epona joven" },
    { ACTOR_DOOR_ANA, "Entrada de gruta" },
    { ACTOR_BG_SPOT02_OBJECTS, "Actores del Cementerio" },
    { ACTOR_BG_HAKA, "Lápida" },
    { ACTOR_MAGIC_WIND, "Viento de Farore" },
    { ACTOR_MAGIC_FIRE, "Fuego de Din" },
    { ACTOR_EN_RU1, "Ruto (niña)" },
    { ACTOR_BOSS_FD2, "Volvagia (en el agujero)" },
    { ACTOR_EN_FD_FIRE, "Ataque de fuego del Flare Dancer" },
    { ACTOR_EN_DH, "Dead Hand" },
    { ACTOR_EN_DHA, "Mano del Dead Hand" },
    { ACTOR_EN_RL, "Rauru" },
    { ACTOR_EN_ENCOUNT1, "Generador de enemigos" },
    { ACTOR_DEMO_DU, "Darunia (escenas)" },
    { ACTOR_DEMO_IM, "Impa (escenas)" },
    { ACTOR_DEMO_TRE_LGT, "Luz del cofre del tesoro" },
    { ACTOR_EN_FW, "Núcleo del Flare Dancer" },
    { ACTOR_BG_VB_SIMA, "Plataforma de Volvagia" },
    { ACTOR_EN_VB_BALL, "Rocas y huesos de Volvagia" },
    { ACTOR_BG_HAKA_MEGANE, "Falsos muros del Templo de las Sombras" },
    { ACTOR_BG_HAKA_MEGANEBG, "Plataformas (Templo de las Sombras)" },
    { ACTOR_BG_HAKA_SHIP, "Barca del río (Templo de las Sombras)" },
    { ACTOR_BG_HAKA_SGAMI, "Trampa de guadañas giratorias" },
    { ACTOR_EN_HEISHI2, "Guardias hylianos" },
    { ACTOR_EN_ENCOUNT2, "Generador de rocas que caen" },
    { ACTOR_EN_FIRE_ROCK, "Rocas ardientes que caen" },
    { ACTOR_EN_BROB, "Bloque de músculo viscoso (tripa de Jabu-Jabu)" },
    { ACTOR_MIR_RAY, "Haz de luz reflectable" },
    { ACTOR_BG_SPOT09_OBJ, "Objetos del Valle Gerudo" },
    { ACTOR_BG_SPOT18_OBJ, "Estatua (sala de Darunia)" },
    { ACTOR_BOSS_VA, "Barinade" },
    { ACTOR_BG_HAKA_TUBO, "Vasija de calavera gigante (Templo de las Sombras)" },
    { ACTOR_BG_HAKA_TRAP, "Trampas del Templo de las Sombras" },
    { ACTOR_BG_HAKA_HUTA, "Tapa de ataúd" },
    { ACTOR_BG_HAKA_ZOU, "Estatua de pájaro y muro (Templo de las Sombras)" },
    { ACTOR_BG_SPOT17_FUNEN, "Cono de humo (Cráter de la Montaña de la Muerte)" },
    { ACTOR_EN_SYATEKI_ITM, "Juego de la galería de tiro" },
    { ACTOR_EN_SYATEKI_MAN, "Hombre de la galería de tiro" },
    { ACTOR_EN_TANA, "Estantes de tienda" },
    { ACTOR_EN_NB, "Nabooru" },
    { ACTOR_BOSS_MO, "Morpha" },
    { ACTOR_EN_SB, "Shell Blade" },
    { ACTOR_EN_BIGOKUTA, "Octo grande" },
    { ACTOR_EN_KAREBABA, "Deku Baba marchita" },
    { ACTOR_BG_BDAN_OBJECTS, "Objetos del interior de la tripa de Jabu-Jabu" },
    { ACTOR_DEMO_SA, "Saria (escenas)" },
    { ACTOR_DEMO_GO, "Gorons (escena)" },
    { ACTOR_EN_IN, "Ingo" },
    { ACTOR_EN_TR, "Koume y Kotake" },
    { ACTOR_BG_SPOT16_BOMBSTONE, "Roca que bloquea la entrada de la Cueva de los dodongos" },
    { ACTOR_BG_HIDAN_KOWARERUKABE, "Muros destruibles con bombas (Templo del Fuego)" },
    { ACTOR_BG_BOMBWALL, "Muro destruible 2D" },
    { ACTOR_BG_SPOT08_ICEBLOCK, "Plataforma de hielo (Fuente Zora)" },
    { ACTOR_EN_RU2, "Ruto (adulta)" },
    { ACTOR_OBJ_DEKUJR, "Brote del Árbol Deku" },
    { ACTOR_BG_MIZU_UZU, "Efecto de remolino (Templo del Agua)" },
    { ACTOR_BG_SPOT06_OBJECTS, "Objetos del Lago Hylia" },
    { ACTOR_BG_ICE_OBJECTS, "Bloque de hielo empujable (Caverna de hielo)" },
    { ACTOR_BG_HAKA_WATER, "Cambiador del nivel de agua del Pozo" },
    { ACTOR_EN_MA2, "Malon (adulta, rancho de Ingo)" },
    { ACTOR_EN_BOM_CHU, "Bombchu" },
    { ACTOR_EN_HORSE_GAME_CHECK, "Minijuegos a caballo" },
    { ACTOR_BOSS_TW, "Twinrova" },
    { ACTOR_EN_RR, "Like-Like" },
    { ACTOR_EN_BA, "Tentáculo (tripa de Jabu-Jabu)" },
    { ACTOR_EN_BX, "Tentáculo electrificado (tripa de Jabu-Jabu)" },
    { ACTOR_EN_ANUBICE, "Anubis (cuerpo)" },
    { ACTOR_EN_ANUBICE_FIRE, "Ataque de fuego de Anubis" },
    { ACTOR_BG_MORI_HASHIGO, "Escalera (Templo del Bosque)" },
    { ACTOR_BG_MORI_HASHIRA4, "Objetos del Templo del Bosque" },
    { ACTOR_BG_MORI_IDOMIZU, "Agua del pozo (Templo del Bosque)" },
    { ACTOR_BG_SPOT16_DOUGHNUT, "Anillo de nubes (Montaña de la Muerte)" },
    { ACTOR_BG_BDAN_SWITCH, "Interruptores (interior de Lord Jabu-Jabu)" },
    { ACTOR_EN_MA1, "Malon (niña)" },
    { ACTOR_BOSS_GANON, "Ganondorf" },
    { ACTOR_BOSS_SST, "Bongo Bongo" },
    { ACTOR_EN_NY, "Pincho (enemigo)" },
    { ACTOR_EN_FR, "Lugar de la canción de las ranas y ranas" },
    { ACTOR_ITEM_SHIELD, "Escudo deku" },
    { ACTOR_BG_ICE_SHELTER, "Hielo rojo" },
    { ACTOR_EN_ICE_HONO, "Llamas de fuego azul" },
    { ACTOR_ITEM_OCARINA, "Ocarina del Tiempo" },
    { ACTOR_MAGIC_DARK, "Amor de Nayru" },
    { ACTOR_DEMO_6K, "Sabios, bolas de luz (escena)" },
    { ACTOR_EN_ANUBICE_TAG, "Generador de Anubis" },
    { ACTOR_BG_HAKA_GATE, "Puzle del giratorio de la verdad (Templo de las Sombras)" },
    { ACTOR_BG_SPOT15_SAKU, "Puerta del Castillo de Hyrule" },
    { ACTOR_BG_JYA_GOROIWA, "Roca rodante (Templo del Espíritu)" },
    { ACTOR_BG_JYA_ZURERUKABE, "Muro de ladrillos corredizo y trepable (Templo del Espíritu)" },
    { ACTOR_BG_JYA_COBRA, "Espejo de cobra giratorio (Templo del Espíritu)" },
    { ACTOR_BG_JYA_KANAAMI, "Puente de rejilla metálica trepable (Templo del Espíritu)" },
    { ACTOR_FISHING, "Dueño del estanque de pesca y peces" },
    { ACTOR_OBJ_OSHIHIKI, "Bloque empujable" },
    { ACTOR_BG_GATE_SHUTTER, "Puerta al Sendero de la Montaña de la Muerte" },
    { ACTOR_EFF_DUST, "Efectos de polvo" },
    { ACTOR_BG_SPOT01_FUSYA, "Aspas del molino (Kakariko)" },
    { ACTOR_BG_SPOT01_IDOHASHIRA, "Viga del pozo (Kakariko)" },
    { ACTOR_BG_SPOT01_IDOMIZU, "Agua del pozo (Kakariko)" },
    { ACTOR_BG_PO_SYOKUDAI, "Antorcha dorada (Hermanas Poe)" },
    { ACTOR_BG_GANON_OTYUKA, "Plataforma que cae (combate contra Ganondorf)" },
    { ACTOR_BG_SPOT15_RRBOX, "Caja de leche" },
    { ACTOR_BG_UMAJUMP, "Valla de obstáculos (Rancho Lon Lon)" },
    { ACTOR_ARROW_FIRE, "Flecha de fuego" },
    { ACTOR_ARROW_ICE, "Flecha de hielo" },
    { ACTOR_ARROW_LIGHT, "Flecha de luz" },
    { ACTOR_ITEM_ETCETERA, "Objetos coleccionables" },
    { ACTOR_OBJ_KIBAKO, "Caja pequeña levantable" },
    { ACTOR_OBJ_TSUBO, "Vasija rompible" },
    { ACTOR_EN_WONDER_ITEM, "Coleccionable invisible" },
    { ACTOR_EN_IK, "Iron Knuckle" },
    { ACTOR_DEMO_IK, "Piezas de armadura de Iron Knuckle (combate de Nabooru, Templo del Espíritu)" },
    { ACTOR_EN_SKJ, "Skullkid" },
    { ACTOR_EN_SKJNEEDLE, "Ataque de agujas de Skull Kid" },
    { ACTOR_EN_G_SWITCH, "Rupia plateada" },
    { ACTOR_DEMO_EXT, "Torbellino mágico" },
    { ACTOR_DEMO_SHD, "Sombra de Bongo Bongo" },
    { ACTOR_EN_DNS, "Mercader deku" },
    { ACTOR_ELF_MSG, "Mensaje de Navi" },
    { ACTOR_EN_HONOTRAP, "Ojo de piedra (trampa de fuego) y llama" },
    { ACTOR_EN_TUBO_TRAP, "Vasija voladora" },
    { ACTOR_OBJ_ICE_POLY, "¿Hielo generado por hielo rojo? Actores congelados" },
    { ACTOR_BG_SPOT03_TAKI, "Cascada del Río Zora" },
    { ACTOR_BG_SPOT07_TAKI, "Cascada y hielo de la Región de los zora" },
    { ACTOR_EN_FZ, "Frezzard" },
    { ACTOR_EN_PO_RELAY, "Fantasma de Dampé" },
    { ACTOR_BG_RELAY_OBJECTS, "Objetos del molino" },
    { ACTOR_EN_DIVING_GAME, "Juego de buceo zora" },
    { ACTOR_EN_KUSA, "Arbusto/hierba" },
    { ACTOR_OBJ_BEAN, "Lugar de plantar judías" },
    { ACTOR_OBJ_BOMBIWA, "Roca marrón destruible con bombas" },
    { ACTOR_OBJ_SWITCH, "Interruptores" },
    { ACTOR_OBJ_ELEVATOR, "Enorme ascensor de piedra" },
    { ACTOR_OBJ_LIFT, "Plataforma cuadrada que se derrumba" },
    { ACTOR_OBJ_HSBLOCK, "Diana de piedra para el gancho" },
    { ACTOR_EN_OKARINA_TAG, "Lugar del pentagrama de la ocarina" },
    { ACTOR_EN_YABUSAME_MARK, "Diana de arquería a caballo" },
    { ACTOR_EN_GOROIWA, "Roca rodante" },
    { ACTOR_EN_EX_RUPPY, "Rupia brillante" },
    { ACTOR_EN_TORYO, "Capataz carpintero" },
    { ACTOR_EN_DAIKU, "Carpinteros" },
    { ACTOR_EN_NWC, "Pollito" },
    { ACTOR_EN_BLKOBJ, "Sala de ilusiones de Link oscuro" },
    { ACTOR_ITEM_INBOX, "Magia de Zelda para abrir puertas" },
    { ACTOR_EN_GE1, "Gerudo de ropa blanca" },
    { ACTOR_OBJ_BLOCKSTOP, "Tope de bloque empujable" },
    { ACTOR_EN_SDA, "Sombra dinámica" },
    { ACTOR_EN_CLEAR_TAG, "Arwing" },
    { ACTOR_EN_NIW_LADY, "Mujer de los cucos" },
    { ACTOR_EN_GM, "Medigoron" },
    { ACTOR_EN_MS, "Vendedor de judías" },
    { ACTOR_EN_HS, "Hijo del carpintero" },
    { ACTOR_BG_INGATE, "Puertas de Ingo (Rancho Lon Lon)" },
    { ACTOR_EN_KANBAN, "Poste indicador cuadrado" },
    { ACTOR_EN_HEISHI3, "Guardia del Castillo de Hyrule" },
    { ACTOR_EN_SYATEKI_NIW, "Cuco (minijuegos)" },
    { ACTOR_EN_ATTACK_NIW, "Cuco (atacando)" },
    { ACTOR_BG_SPOT01_IDOSOKO, "Piedra que bloquea la entrada al Pozo" },
    { ACTOR_EN_SA, "Saria" },
    { ACTOR_EN_WONDER_TALK, "Lugar examinable (Navi verde)" },
    { ACTOR_BG_GJYO_BRIDGE, "Puente arcoíris al Castillo de Ganon" },
    { ACTOR_EN_DS, "Abuela de la tienda de pociones" },
    { ACTOR_EN_MK, "Profesor del lago" },
    { ACTOR_EN_BOM_BOWL_MAN, "Mujer de la bolera de bombuchus" },
    { ACTOR_EN_BOM_BOWL_PIT, "Diana final de la bolera de bombuchus" },
    { ACTOR_EN_OWL, "Kaepora Gaebora" },
    { ACTOR_EN_ISHI, "Roca levantable" },
    { ACTOR_OBJ_HANA, "Flores, rocas y hierba no interactuables" },
    { ACTOR_OBJ_LIGHTSWITCH, "Interruptor del emblema del sol (Templo del Espíritu)" },
    { ACTOR_OBJ_MURE2, "Grupos de rocas/arbustos" },
    { ACTOR_EN_GO, "Gorons 1" },
    { ACTOR_EN_FU, "Hombre del molino" },
    { ACTOR_EN_CHANGER, "Minijuego de los cofres del tesoro" },
    { ACTOR_BG_JYA_MEGAMI, "Cara de estatua (Templo del Espíritu)" },
    { ACTOR_BG_JYA_LIFT, "Plataforma de cadenas (Templo del Espíritu)" },
    { ACTOR_BG_JYA_BIGMIRROR, "Gran espejo circular (Templo del Espíritu)" },
    { ACTOR_BG_JYA_BOMBCHUIWA, "Roca que bloquea la luz (Templo del Espíritu)" },
    { ACTOR_BG_JYA_AMISHUTTER, "Reja metálica circular corredera (Templo del Espíritu)" },
    { ACTOR_BG_JYA_BOMBIWA, "Muro de roca destruible con bombas (Templo del Espíritu)" },
    { ACTOR_BG_SPOT18_BASKET, "Vasija goron grande" },
    { ACTOR_EN_GANON_ORGAN, "Órgano de Ganon y alrededores" },
    { ACTOR_EN_SIOFUKI, "Surtidor de agua" },
    { ACTOR_EN_STREAM, "Remolino de agua" },
    { ACTOR_EN_MM, "Corredor (época niño)" },
    { ACTOR_EN_KO, "Niños kokiri" },
    { ACTOR_EN_KZ, "Rey Zora" },
    { ACTOR_EN_WEATHER_TAG, "Efectos climáticos por proximidad" },
    { ACTOR_BG_SST_FLOOR, "Tambor de Bongo Bongo" },
    { ACTOR_EN_ANI, "Hombre del tejado de Kakariko" },
    { ACTOR_EN_EX_ITEM, "Objetos mostrados en minijuegos" },
    { ACTOR_BG_JYA_IRONOBJ, "Elementos de la sala de los Iron Knuckle (Templo del Espíritu)" },
    { ACTOR_EN_JS, "Vendedor de alfombras mágicas" },
    { ACTOR_EN_JSJUTAN, "Alfombra del vendedor de alfombras" },
    { ACTOR_EN_CS, "Chico del Cementerio" },
    { ACTOR_EN_MD, "Mido" },
    { ACTOR_EN_HY, "PNJ de la plaza" },
    { ACTOR_EN_GANON_MANT, "Capa de Ganondorf" },
    { ACTOR_EN_OKARINA_EFFECT, "Gestor de tormenta de la Canción de la tormenta" },
    { ACTOR_EN_MAG, "Gestor de la pantalla de título" },
    { ACTOR_DOOR_GERUDO, "Puerta de celda (Fortaleza gerudo)" },
    { ACTOR_ELF_MSG2, "Lugar de información de Navi (apuntable, verde)" },
    { ACTOR_DEMO_GT, "Derrumbe de la torre de Ganon (objetos de escena)" },
    { ACTOR_EN_PO_FIELD, "Punto de aparición de Poe grande/pequeño" },
    { ACTOR_EFC_ERUPC, "Fuente de partículas de lava (panorama de la Montaña de la Muerte)" },
    { ACTOR_BG_ZG, "Barrotes metálicos (Castillo de Ganon)" },
    { ACTOR_EN_HEISHI4, "Guardia de Hyrule" },
    { ACTOR_EN_ZL3, "Zelda adulta" },
    { ACTOR_BOSS_GANON2, "Ganon" },
    { ACTOR_EN_KAKASI, "Pierre el espantapájaros" },
    { ACTOR_EN_TAKARA_MAN, "Dueño de los cofres del tesoro" },
    { ACTOR_OBJ_MAKEOSHIHIKI, "Puzles de bloques empujables" },
    { ACTOR_OCEFF_SPOT, "Efecto de la Canción del Sol" },
    { ACTOR_END_TITLE, "Mensaje de FIN" },
    { ACTOR_EN_TORCH, "Cofre del tesoro de gruta" },
    { ACTOR_DEMO_EC, "Juerguistas de los créditos en Lon Lon" },
    { ACTOR_SHOT_SUN, "Hitbox del sol del Lago Hylia y generador de hada grande" },
    { ACTOR_EN_DY_EXTRA, "Haces en espiral (fuentes de Grandes hadas)" },
    { ACTOR_EN_WONDER_TALK2, "Lugar de diálogo" },
    { ACTOR_EN_GE2, "Gerudo de patrulla" },
    { ACTOR_OBJ_ROOMTIMER, "Temporizador de sala" },
    { ACTOR_EN_SSH, "Gente maldecida por las Skulltulas" },
    { ACTOR_EN_STH, "Gente liberada de las Skulltulas" },
    { ACTOR_OCEFF_WIPE, "Efecto de ocarina de la Nana de Zelda y la Canción del tiempo" },
    { ACTOR_OCEFF_STORM, "Efecto de ocarina de la Canción de la tormenta" },
    { ACTOR_EN_WEIYER, "Stinger (agua)" },
    { ACTOR_BG_SPOT05_SOKO, "Objetos de la Pradera Sagrada" },
    { ACTOR_BG_JYA_1FLIFT, "Ascensor de piedra (Templo del Espíritu)" },
    { ACTOR_BG_JYA_HAHENIRON, "Trozos de la silla y el pilar del Iron Knuckle" },
    { ACTOR_BG_SPOT12_GATE, "Puerta de madera de la Fortaleza gerudo" },
    { ACTOR_BG_SPOT12_SAKU, "Puerta de la zona de entrenamiento de la Fortaleza gerudo" },
    { ACTOR_EN_HINTNUTS, "Mercaderes deku de pistas (Árbol Deku)" },
    { ACTOR_EN_NUTSBALL, "Ataque de nueces del mercader deku" },
    { ACTOR_BG_SPOT00_BREAK, "Puente levadizo roto y vallas" },
    { ACTOR_EN_SHOPNUTS, "Mercader deku en el suelo" },
    { ACTOR_EN_IT, "Coleccionables del minijuego de Dampé" },
    { ACTOR_EN_GELDB, "Guerrera gerudo" },
    { ACTOR_OCEFF_WIPE2, "Efecto de ocarina de la Canción de Epona" },
    { ACTOR_OCEFF_WIPE3, "Efecto de ocarina de la Canción de Saria" },
    { ACTOR_EN_NIW_GIRL, "Chica persiguiendo al cuco" },
    { ACTOR_EN_DOG, "Perro" },
    { ACTOR_EN_SI, "Símbolo de skulltula dorada" },
    { ACTOR_BG_SPOT01_OBJECTS2, "Objetos de Kakariko" },
    { ACTOR_OBJ_COMB, "Colmena" },
    { ACTOR_BG_SPOT11_BAKUDANKABE, "Muro destruible (Coloso del desierto)" },
    { ACTOR_OBJ_KIBAKO2, "Caja grande" },
    { ACTOR_EN_DNT_DEMO, "Activador del panel de máscaras deku" },
    { ACTOR_EN_DNT_JIJI, "Juez principal del panel de máscaras deku" },
    { ACTOR_EN_DNT_NOMAL, "¿Juego Deku?" },
    { ACTOR_EN_GUEST, "Cliente de la tienda de máscaras" },
    { ACTOR_BG_BOM_GUARD, "Zona de puntería de la bolera de bombuchus" },
    { ACTOR_EN_HS2, "Hijo del carpintero (época niño)" },
    { ACTOR_DEMO_KEKKAI, "Barreras mágicas de la torre de Ganon" },
    { ACTOR_BG_SPOT08_BAKUDANKABE, "Muro destruible (Fuente Zora)" },
    { ACTOR_BG_SPOT17_BAKUDANKABE, "Muro destruible (Cráter de la Montaña de la Muerte)" },
    { ACTOR_OBJ_MURE3, "Patrones de rupias" },
    { ACTOR_EN_TG, "Enamorados entrelazados (Honey y Darling)" },
    { ACTOR_EN_MU, "Aldeanos regateando" },
    { ACTOR_EN_GO2, "Gorons 2" },
    { ACTOR_EN_WF, "Wolfos" },
    { ACTOR_EN_SKB, "Stalchild" },
    { ACTOR_DEMO_GJ, "Escombros del combate contra Ganon" },
    { ACTOR_DEMO_GEFF, "Fragmento de escombros de la torre de Ganon" },
    { ACTOR_BG_GND_FIREMEIRO, "Plataforma de lava que se hunde (Castillo de Ganon)" },
    { ACTOR_BG_GND_DARKMEIRO, "Bloque transparente" },
    { ACTOR_BG_GND_SOULMEIRO, "Agujero del techo bloqueado por telaraña (interior del Castillo de Ganon)" },
    { ACTOR_BG_GND_NISEKABE, "Falsos muros del Castillo de Ganon" },
    { ACTOR_BG_GND_ICEBLOCK, "Bloque de hielo cuadrado empujable (interior del Castillo de Ganon)" },
    { ACTOR_EN_GB, "Coleccionista de Poes y alrededores" },
    { ACTOR_EN_GS, "Piedra chismosa" },
    { ACTOR_BG_MIZU_BWALL, "Muro de piedra destruible con bombas" },
    { ACTOR_BG_MIZU_SHUTTER, "Verja metálica (Templo del Agua)" },
    { ACTOR_EN_DAIKU_KAKARIKO, "Carpinteros (Kakariko)" },
    { ACTOR_BG_BOWL_WALL, "Muro de la bolera de bombuchus" },
    { ACTOR_EN_WALL_TUBO, "Dianas de la bolera de bombuchus" },
    { ACTOR_EN_PO_DESERT, "Poe guía (Desierto encantado)" },
    { ACTOR_EN_CROW, "Guay" },
    { ACTOR_DOOR_KILLER, "Puerta falsa" },
    { ACTOR_BG_SPOT11_OASIS, "Oasis (Coloso del desierto)" },
    { ACTOR_BG_SPOT18_FUTA, "Tapa de la vasija goron" },
    { ACTOR_BG_SPOT18_SHUTTER, "Puertas correderas (Ciudad Goron)" },
    { ACTOR_EN_MA3, "Malon (adulta, Rancho Lon Lon)" },
    { ACTOR_EN_COW, "Vaca" },
    { ACTOR_BG_ICE_TURARA, "Carámbanos" },
    { ACTOR_BG_ICE_SHUTTER, "Barras de hielo verticales (Caverna de hielo)" },
    { ACTOR_EN_KAKASI2, "Aparición de Pierre el espantapájaros" },
    { ACTOR_EN_KAKASI3, "Bonooru el espantapájaros" },
    { ACTOR_OCEFF_WIPE4, "Efecto de ocarina del Son del espantapájaros" },
    { ACTOR_EN_EG, "Activador de caída al vacío (derrumbe de la torre)" },
    { ACTOR_BG_MENKURI_NISEKABE, "Falsos muros de piedra (Centro de Instrucción Gerudo)" },
    { ACTOR_EN_ZO, "Zora" },
    { ACTOR_OBJ_MAKEKINSUTA, "Skulltula surgiendo del lugar de judías" },
    { ACTOR_EN_GE3, "Líder de la Fortaleza gerudo" },
    { ACTOR_OBJ_TIMEBLOCK, "Bloque del tiempo" },
    { ACTOR_OBJ_HAMISHI, "Roca de bronce" },
    { ACTOR_EN_ZL4, "Zelda (niña)" },
    { ACTOR_EN_MM2, "Corredor (época adulto)" },
    { ACTOR_BG_JYA_BLOCK, "Bloque plateado (época niño)" },
    { ACTOR_OBJ_WARP2BLOCK, "Lugar de información de Navi (verde, bloque del tiempo)" }
};

const std::string GetActorDescription(u16 id) {
    return actorDescriptions[id] != NULL ? actorDescriptions[id] : "???";
}

template <typename T> void DrawGroupWithBorder(T&& drawFunc) {
    // First group encapsulates the inner portion and border
    ImGui::BeginGroup();

    ImVec2 padding = ImGui::GetStyle().FramePadding;
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(p0.x + padding.x, p0.y + padding.y));

    // Second group encapsulates just the inner portion
    ImGui::BeginGroup();

    drawFunc();

    ImGui::Dummy(padding);
    ImGui::EndGroup();

    ImVec2 p1 = ImGui::GetItemRectMax();
    p1.x += padding.x;
    ImVec4 borderCol = ImGui::GetStyle().Colors[ImGuiCol_Border];
    ImGui::GetWindowDrawList()->AddRect(
        p0, p1, IM_COL32(borderCol.x * 255, borderCol.y * 255, borderCol.z * 255, borderCol.w * 255));

    ImGui::EndGroup();
}

void PopulateActorDropdown(int i, std::vector<Actor*>& data) {
    if (data.size() != 0) {
        data.clear();
    }
    if (gPlayState != nullptr) {
        ActorListEntry currList = gPlayState->actorCtx.actorLists[i];
        Actor* currAct = currList.head;
        if (currAct != nullptr) {
            while (currAct != nullptr) {
                data.push_back(currAct);
                currAct = currAct->next;
            }
        }
    }
}


void DrawActorViewer(bool& open) {
    if (!open) {
        CVarSetInteger("gActorViewerEnabled", 0);
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Visor de actores", &open, ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::End();
        return;
    }

    static Actor* display;
    static Actor empty{};
    static Actor* fetch = NULL;
    static ActorInfo newActor = {0,0, {0, 0, 0}, {0, 0, 0}};
    static ActorOverlay* dispOverlay;
    static bool needs_reset = false;
    static ImU16 one = 1;
    static int actor;
    static int category = 0;
    static RetrievalMethod rm;
    static std::string filler = "Selecciona uno";
    static std::vector<Actor*> list;
    static u16 lastSceneId = 0;

    if (gPlayState != nullptr) {
        needs_reset = lastSceneId != gPlayState->sceneNum;
        if (needs_reset) {
            display = &empty;
            fetch = nullptr;
            dispOverlay = nullptr;
            actor = category = 0;
            filler = "Selecciona uno";
            list.clear();
            needs_reset = false;
        }
        lastSceneId = gPlayState->sceneNum;
        if (ImGui::BeginCombo("Tipo de actor", acMapping[category])) {
            for (int i = 0; i < acMapping.size(); i++) {
                if (ImGui::Selectable(acMapping[i])) {
                    category = i;
                    PopulateActorDropdown(category, list);
                    break;
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Actor", filler.c_str())) {
            if (gPlayState != nullptr && lastSceneId != gPlayState->sceneNum) {
                PopulateActorDropdown(category, list);
                lastSceneId = gPlayState->sceneNum;
            }
            for (int i = 0; i < list.size(); i++) {
                std::string label = std::to_string(i) + ": " + list[i]->overlayEntry->name;
                std::string description = GetActorDescription(list[i]->id);
                if (description != "")
                    label += " (" + description + ")";

                if (ImGui::Selectable(label.c_str())) {
                    rm = LIST;
                    display = list[i];
                    actor = i;
                    filler = label;
                    break;
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::TreeNode("Actor seleccionado")) {
            dispOverlay = display->overlayEntry;
            DrawGroupWithBorder([&]() {
                ImGui::Text("Nombre: %s", dispOverlay != nullptr ? dispOverlay->name : "???");
                ImGui::Text("Descripción: %s", dispOverlay != nullptr ? GetActorDescription(display->id).c_str() : "???");
                ImGui::Text("Categoría: %s", dispOverlay != nullptr ? acMapping[display->category] : "???");
                ImGui::Text("ID: %d", display->id);
                ImGui::Text("Parámetros: %d", display->params);
            });

            ImGui::PushItemWidth(ImGui::GetFontSize() * 6);

            DrawGroupWithBorder([&]() {
                ImGui::Text("Posición del actor");
                ImGui::InputScalar("pos. x", ImGuiDataType_Float, &display->world.pos.x);
                ImGui::SameLine();
                ImGui::InputScalar("pos. y", ImGuiDataType_Float, &display->world.pos.y);
                ImGui::SameLine();
                ImGui::InputScalar("pos. z", ImGuiDataType_Float, &display->world.pos.z);
            });

            DrawGroupWithBorder([&]() {
                ImGui::Text("Rotación del actor");
                ImGui::InputScalar("rot. x", ImGuiDataType_S16, &display->world.rot.x);
                ImGui::SameLine();
                ImGui::InputScalar("rot. y", ImGuiDataType_S16, &display->world.rot.y);
                ImGui::SameLine();
                ImGui::InputScalar("rot. z", ImGuiDataType_S16, &display->world.rot.z);
            });

            if (display->category == ACTORCAT_BOSS || display->category == ACTORCAT_ENEMY) {
                ImGui::InputScalar("Salud del enemigo", ImGuiDataType_U8, &display->colChkInfo.health);
                UIWidgets::InsertHelpHoverText("¡Puede que algunos actores no lo usen!");
            }

            DrawGroupWithBorder([&]() {
                ImGui::Text("flags");
                UIWidgets::DrawFlagArray32("flags", display->flags);
            });

            ImGui::SameLine();

            DrawGroupWithBorder([&]() {
                ImGui::Text("bgCheckFlags");
                UIWidgets::DrawFlagArray16("bgCheckFlags", display->bgCheckFlags);
            });

            if (ImGui::Button("Actualizar")) {
                PopulateActorDropdown(category, list);
                switch (rm) {
                    case INTERACT:
                    case HELD:
                    case TARGET:
                        display = fetch;
                        break;
                    case LIST:
                        display = list[actor];
                        break;
                    default:
                        break;
                }
            }

            if (ImGui::Button("Ir al actor")) {
                Player* player = GET_PLAYER(gPlayState);
                Math_Vec3f_Copy(&player->actor.world.pos, &display->world.pos);
                Math_Vec3f_Copy(&player->actor.home.pos, &player->actor.world.pos);
            }

            if (ImGui::Button("Obtener del objetivo")) {
                Player* player = GET_PLAYER(gPlayState);
                fetch = player->targetActor;
                if (fetch != NULL) {
                    display = fetch;
                    category = fetch->category;
                    PopulateActorDropdown(category, list);
                    rm = TARGET;
                }
            }
            UIWidgets::InsertHelpHoverText("Obtiene el actor con la flecha de apuntado encima. Con enemigos quizá necesites C-Arriba");
            if (ImGui::Button("Obtener del objeto en mano")) {
                Player* player = GET_PLAYER(gPlayState);
                fetch = player->heldActor;
                if (fetch != NULL) {
                    display = fetch;
                    category = fetch->category;
                    PopulateActorDropdown(category, list);
                    rm = HELD;
                }
            }
            UIWidgets::InsertHelpHoverText("Obtiene el actor que Link sostiene");
            if (ImGui::Button("Obtener de la interacción")) {
                Player* player = GET_PLAYER(gPlayState);
                fetch = player->interactRangeActor;
                if (fetch != NULL) {
                    display = fetch;
                    category = fetch->category;
                    PopulateActorDropdown(category, list);
                    rm = INTERACT;
                }
            }
            UIWidgets::InsertHelpHoverText("Obtiene el actor del \"rango de interacción\"");

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Nuevo...")) {
            ImGui::PushItemWidth(ImGui::GetFontSize() * 10);

            ImGui::Text(GetActorDescription(newActor.id).c_str());
            ImGui::InputScalar("ID", ImGuiDataType_S16, &newActor.id, &one);
            ImGui::InputScalar("params", ImGuiDataType_S16, &newActor.params, &one);

            ImGui::PushItemWidth(ImGui::GetFontSize() * 6);

            DrawGroupWithBorder([&]() {
                ImGui::Text("Posición del nuevo actor");
                ImGui::InputScalar("posX", ImGuiDataType_Float, &newActor.pos.x);
                ImGui::SameLine();
                ImGui::InputScalar("posY", ImGuiDataType_Float, &newActor.pos.y);
                ImGui::SameLine();
                ImGui::InputScalar("posZ", ImGuiDataType_Float, &newActor.pos.z);
            });

            DrawGroupWithBorder([&]() {
                ImGui::Text("Rotación del nuevo actor");
                ImGui::InputScalar("rotX", ImGuiDataType_S16, &newActor.rot.x);
                ImGui::SameLine();
                ImGui::InputScalar("rotY", ImGuiDataType_S16, &newActor.rot.y);
                ImGui::SameLine();
                ImGui::InputScalar("rotZ", ImGuiDataType_S16, &newActor.rot.z);
            });

            if (ImGui::Button("Obtener de Link")) {
                Player* player = GET_PLAYER(gPlayState);
                Vec3f newPos = player->actor.world.pos;
                Vec3s newRot = player->actor.world.rot;
                newActor.pos = newPos;
                newActor.rot = newRot;
            }

            if (ImGui::Button("Generar")) {
                if (newActor.id >= 0 && newActor.id < ACTOR_ID_MAX && gActorOverlayTable[newActor.id].initInfo != NULL) {
                    Actor_Spawn(&gPlayState->actorCtx, gPlayState, newActor.id, newActor.pos.x, newActor.pos.y,
                                newActor.pos.z, newActor.rot.x, newActor.rot.y, newActor.rot.z, newActor.params, 0);
                } else {
                    func_80078884(NA_SE_SY_ERROR);
                }
            }

            if (ImGui::Button("Generar como hijo")) {
                Actor* parent = display;
                if (parent != NULL) {
                    if (newActor.id >= 0 && newActor.id < ACTOR_ID_MAX &&
                        gActorOverlayTable[newActor.id].initInfo != NULL) {
                        Actor_SpawnAsChild(&gPlayState->actorCtx, parent, gPlayState, newActor.id, newActor.pos.x,
                                           newActor.pos.y, newActor.pos.z, newActor.rot.x, newActor.rot.y,
                                           newActor.rot.z, newActor.params);
                    } else {
                        func_80078884(NA_SE_SY_ERROR);                    
                    }
                }
            }

            if (ImGui::Button("Restablecer")) {
                newActor = { 0, 0, { 0, 0, 0 }, { 0, 0, 0 } };
            }

            ImGui::TreePop();
        }
    } else {
        ImGui::Text("¡Se necesita el contexto global para la información de actores!");
        if (needs_reset) {
            fetch = nullptr;
            dispOverlay = nullptr;
            actor = category = 0;
            filler = "Selecciona uno";
            list.clear();
            needs_reset = false;
        }
    }
    

    ImGui::End();
}

void InitActorViewer() {
    SohImGui::AddWindow("Developer Tools", "Actor Viewer", DrawActorViewer);
}
