#include "debugSaveEditor.h"
#include "../../util.h"
#include "../../OTRGlobals.h"
#include <ImGuiImpl.h>
#include "../../UIWidgets.hpp"

#include <spdlog/fmt/fmt.h>
#include <array>
#include <bit>
#include <map>
#include <string>
#include <libultraship/bridge.h>

extern "C" {
#include <z64.h>
#include "variables.h"
#include "functions.h"
#include "macros.h"
#include "soh/Enhancements/randomizer/adult_trade_shuffle.h"
extern PlayState* gPlayState;

#include "textures/icon_item_static/icon_item_static.h"
#include "textures/icon_item_24_static/icon_item_24_static.h"
}

typedef struct {
    uint32_t id;
    std::string name;
    std::string nameFaded;
    std::string texturePath;
} ItemMapEntry;

#define ITEM_MAP_ENTRY(id)                              \
    {                                                   \
        id, {                                           \
            id, #id, #id "_Faded", static_cast<char*>(gItemIcons[id]) \
        }                                               \
    }

// Maps items ids to info for use in ImGui
std::map<uint32_t, ItemMapEntry> itemMapping = {
    ITEM_MAP_ENTRY(ITEM_STICK),
    ITEM_MAP_ENTRY(ITEM_NUT),
    ITEM_MAP_ENTRY(ITEM_BOMB),
    ITEM_MAP_ENTRY(ITEM_BOW),
    ITEM_MAP_ENTRY(ITEM_ARROW_FIRE),
    ITEM_MAP_ENTRY(ITEM_DINS_FIRE),
    ITEM_MAP_ENTRY(ITEM_SLINGSHOT),
    ITEM_MAP_ENTRY(ITEM_OCARINA_FAIRY),
    ITEM_MAP_ENTRY(ITEM_OCARINA_TIME),
    ITEM_MAP_ENTRY(ITEM_BOMBCHU),
    ITEM_MAP_ENTRY(ITEM_HOOKSHOT),
    ITEM_MAP_ENTRY(ITEM_LONGSHOT),
    ITEM_MAP_ENTRY(ITEM_ARROW_ICE),
    ITEM_MAP_ENTRY(ITEM_FARORES_WIND),
    ITEM_MAP_ENTRY(ITEM_BOOMERANG),
    ITEM_MAP_ENTRY(ITEM_LENS),
    ITEM_MAP_ENTRY(ITEM_BEAN),
    ITEM_MAP_ENTRY(ITEM_HAMMER),
    ITEM_MAP_ENTRY(ITEM_ARROW_LIGHT),
    ITEM_MAP_ENTRY(ITEM_NAYRUS_LOVE),
    ITEM_MAP_ENTRY(ITEM_BOTTLE),
    ITEM_MAP_ENTRY(ITEM_POTION_RED),
    ITEM_MAP_ENTRY(ITEM_POTION_GREEN),
    ITEM_MAP_ENTRY(ITEM_POTION_BLUE),
    ITEM_MAP_ENTRY(ITEM_FAIRY),
    ITEM_MAP_ENTRY(ITEM_FISH),
    ITEM_MAP_ENTRY(ITEM_MILK_BOTTLE),
    ITEM_MAP_ENTRY(ITEM_LETTER_RUTO),
    ITEM_MAP_ENTRY(ITEM_BLUE_FIRE),
    ITEM_MAP_ENTRY(ITEM_BUG),
    ITEM_MAP_ENTRY(ITEM_BIG_POE),
    ITEM_MAP_ENTRY(ITEM_MILK_HALF),
    ITEM_MAP_ENTRY(ITEM_POE),
    ITEM_MAP_ENTRY(ITEM_WEIRD_EGG),
    ITEM_MAP_ENTRY(ITEM_CHICKEN),
    ITEM_MAP_ENTRY(ITEM_LETTER_ZELDA),
    ITEM_MAP_ENTRY(ITEM_MASK_KEATON),
    ITEM_MAP_ENTRY(ITEM_MASK_SKULL),
    ITEM_MAP_ENTRY(ITEM_MASK_SPOOKY),
    ITEM_MAP_ENTRY(ITEM_MASK_BUNNY),
    ITEM_MAP_ENTRY(ITEM_MASK_GORON),
    ITEM_MAP_ENTRY(ITEM_MASK_ZORA),
    ITEM_MAP_ENTRY(ITEM_MASK_GERUDO),
    ITEM_MAP_ENTRY(ITEM_MASK_TRUTH),
    ITEM_MAP_ENTRY(ITEM_SOLD_OUT),
    ITEM_MAP_ENTRY(ITEM_POCKET_EGG),
    ITEM_MAP_ENTRY(ITEM_POCKET_CUCCO),
    ITEM_MAP_ENTRY(ITEM_COJIRO),
    ITEM_MAP_ENTRY(ITEM_ODD_MUSHROOM),
    ITEM_MAP_ENTRY(ITEM_ODD_POTION),
    ITEM_MAP_ENTRY(ITEM_SAW),
    ITEM_MAP_ENTRY(ITEM_SWORD_BROKEN),
    ITEM_MAP_ENTRY(ITEM_PRESCRIPTION),
    ITEM_MAP_ENTRY(ITEM_FROG),
    ITEM_MAP_ENTRY(ITEM_EYEDROPS),
    ITEM_MAP_ENTRY(ITEM_CLAIM_CHECK),
    ITEM_MAP_ENTRY(ITEM_BOW_ARROW_FIRE),
    ITEM_MAP_ENTRY(ITEM_BOW_ARROW_ICE),
    ITEM_MAP_ENTRY(ITEM_BOW_ARROW_LIGHT),
    ITEM_MAP_ENTRY(ITEM_SWORD_KOKIRI),
    ITEM_MAP_ENTRY(ITEM_SWORD_MASTER),
    ITEM_MAP_ENTRY(ITEM_SWORD_BGS),
    ITEM_MAP_ENTRY(ITEM_SHIELD_DEKU),
    ITEM_MAP_ENTRY(ITEM_SHIELD_HYLIAN),
    ITEM_MAP_ENTRY(ITEM_SHIELD_MIRROR),
    ITEM_MAP_ENTRY(ITEM_TUNIC_KOKIRI),
    ITEM_MAP_ENTRY(ITEM_TUNIC_GORON),
    ITEM_MAP_ENTRY(ITEM_TUNIC_ZORA),
    ITEM_MAP_ENTRY(ITEM_BOOTS_KOKIRI),
    ITEM_MAP_ENTRY(ITEM_BOOTS_IRON),
    ITEM_MAP_ENTRY(ITEM_BOOTS_HOVER),
    ITEM_MAP_ENTRY(ITEM_BULLET_BAG_30),
    ITEM_MAP_ENTRY(ITEM_BULLET_BAG_40),
    ITEM_MAP_ENTRY(ITEM_BULLET_BAG_50),
    ITEM_MAP_ENTRY(ITEM_QUIVER_30),
    ITEM_MAP_ENTRY(ITEM_QUIVER_40),
    ITEM_MAP_ENTRY(ITEM_QUIVER_50),
    ITEM_MAP_ENTRY(ITEM_BOMB_BAG_20),
    ITEM_MAP_ENTRY(ITEM_BOMB_BAG_30),
    ITEM_MAP_ENTRY(ITEM_BOMB_BAG_40),
    ITEM_MAP_ENTRY(ITEM_BRACELET),
    ITEM_MAP_ENTRY(ITEM_GAUNTLETS_SILVER),
    ITEM_MAP_ENTRY(ITEM_GAUNTLETS_GOLD),
    ITEM_MAP_ENTRY(ITEM_SCALE_SILVER),
    ITEM_MAP_ENTRY(ITEM_SCALE_GOLDEN),
    ITEM_MAP_ENTRY(ITEM_SWORD_KNIFE),
    ITEM_MAP_ENTRY(ITEM_WALLET_ADULT),
    ITEM_MAP_ENTRY(ITEM_WALLET_GIANT),
    ITEM_MAP_ENTRY(ITEM_SEEDS),
    ITEM_MAP_ENTRY(ITEM_FISHING_POLE),
    ITEM_MAP_ENTRY(ITEM_KEY_BOSS),
    ITEM_MAP_ENTRY(ITEM_COMPASS),
    ITEM_MAP_ENTRY(ITEM_DUNGEON_MAP),
    ITEM_MAP_ENTRY(ITEM_KEY_SMALL),
    ITEM_MAP_ENTRY(ITEM_HEART_CONTAINER),
    ITEM_MAP_ENTRY(ITEM_HEART_PIECE),
    ITEM_MAP_ENTRY(ITEM_MAGIC_SMALL),
    ITEM_MAP_ENTRY(ITEM_MAGIC_LARGE)
};

std::map<uint32_t, ItemMapEntry> gregMapping = {
    {ITEM_RUPEE_GREEN, {ITEM_RUPEE_GREEN, "ITEM_RUPEE_GREEN", "ITEM_RUPEE_GREEN_Faded", "__OTR__textures/parameter_static/gRupeeCounterIconTex"}}
};

// Maps entries in the GS flag array to the area name it represents
std::vector<std::string> gsMapping = {
    "Gran Árbol Deku",
    "Cueva de los dodongos",
    "Tripa de Jabu-Jabu",
    "Templo del Bosque",
    "Templo del Fuego",
    "Templo del Agua",
    "Templo del Espíritu",
    "Templo de las Sombras",
    "Pozo de Kakariko",
    "Caverna de hielo",
    "Llanura de Hyrule",
    "Rancho Lon Lon",
    "Bosque Kokiri",
    "Bosque Perdido, Pradera Sagrada",
    "Plaza del castillo y Castillo de Ganon",
    "Sendero de la Montaña de la Muerte, Ciudad Goron",
    "Kakariko",
    "Fuente Zora, Río Zora",
    "Lago Hylia",
    "Valle Gerudo",
    "Fortaleza gerudo",
    "Coloso del desierto, Desierto encantado",
};

extern "C" u8 gAreaGsFlags[];

extern "C" u8 gAmmoItems[];

// Modification of gAmmoItems that replaces ITEM_NONE with the item in inventory slot it represents
u8 gAllAmmoItems[] = {
    ITEM_STICK,     ITEM_NUT,          ITEM_BOMB,    ITEM_BOW,      ITEM_ARROW_FIRE, ITEM_DINS_FIRE,
    ITEM_SLINGSHOT, ITEM_OCARINA_TIME, ITEM_BOMBCHU, ITEM_LONGSHOT, ITEM_ARROW_ICE,  ITEM_FARORES_WIND,
    ITEM_BOOMERANG, ITEM_LENS,         ITEM_BEAN,    ITEM_HAMMER,
};

typedef struct {
    uint32_t id;
    std::string name;
    std::string nameFaded;
    std::string texturePath;
} QuestMapEntry;

#define QUEST_MAP_ENTRY(id, tex)                              \
    {                                                   \
        id, {                                           \
            id, #id, #id "_Faded", tex \
        }                                               \
    }

// Maps quest items ids to info for use in ImGui
std::map<uint32_t, QuestMapEntry> questMapping = {
    QUEST_MAP_ENTRY(QUEST_MEDALLION_FOREST, dgForestMedallionIconTex),
    QUEST_MAP_ENTRY(QUEST_MEDALLION_FIRE, dgFireMedallionIconTex),
    QUEST_MAP_ENTRY(QUEST_MEDALLION_WATER, dgWaterMedallionIconTex),
    QUEST_MAP_ENTRY(QUEST_MEDALLION_SPIRIT, dgSpiritMedallionIconTex),
    QUEST_MAP_ENTRY(QUEST_MEDALLION_SHADOW, dgShadowMedallionIconTex),
    QUEST_MAP_ENTRY(QUEST_MEDALLION_LIGHT, dgLightMedallionIconTex),
    QUEST_MAP_ENTRY(QUEST_KOKIRI_EMERALD, dgKokiriEmeraldIconTex),
    QUEST_MAP_ENTRY(QUEST_GORON_RUBY, dgGoronRubyIconTex),
    QUEST_MAP_ENTRY(QUEST_ZORA_SAPPHIRE, dgZoraSapphireIconTex),
    QUEST_MAP_ENTRY(QUEST_STONE_OF_AGONY, dgStoneOfAgonyIconTex),
    QUEST_MAP_ENTRY(QUEST_GERUDO_CARD, dgGerudosCardIconTex),
    QUEST_MAP_ENTRY(QUEST_SKULL_TOKEN, dgGoldSkulltulaIconTex),
};

typedef struct {
    uint32_t id;
    std::string name;
    std::string nameFaded;
    ImVec4 color;
} SongMapEntry;

#define SONG_MAP_ENTRY(id, r, g, b)       \
    {                                  \
            id, #id, #id "_Faded", ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f) \
    }

// Maps song ids to info for use in ImGui
std::array<SongMapEntry, 12> songMapping = { {
    SONG_MAP_ENTRY(QUEST_SONG_LULLABY,  224, 107, 255),
    SONG_MAP_ENTRY(QUEST_SONG_EPONA,    255, 195, 60),
    SONG_MAP_ENTRY(QUEST_SONG_SARIA,    127, 255, 137),
    SONG_MAP_ENTRY(QUEST_SONG_SUN,      255, 255, 60),
    SONG_MAP_ENTRY(QUEST_SONG_TIME,     119, 236, 255),
    SONG_MAP_ENTRY(QUEST_SONG_STORMS,   165, 165, 165),
    SONG_MAP_ENTRY(QUEST_SONG_MINUET,   150, 255, 100),
    SONG_MAP_ENTRY(QUEST_SONG_BOLERO,   255, 80,  40),
    SONG_MAP_ENTRY(QUEST_SONG_SERENADE, 100, 150, 255),
    SONG_MAP_ENTRY(QUEST_SONG_REQUIEM,  255, 160, 0),
    SONG_MAP_ENTRY(QUEST_SONG_NOCTURNE, 255, 100, 255),
    SONG_MAP_ENTRY(QUEST_SONG_PRELUDE,  255, 240, 100),
} };

#define VANILLA_SONG_MAP_ENTRY(id, r, g, b)       \
    {                                  \
            id, #id "_Vanilla", #id "_Vanilla_Faded", ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f) \
    }

// Maps song ids to info for use in ImGui
std::array<SongMapEntry, 12> vanillaSongMapping = { {
    VANILLA_SONG_MAP_ENTRY(QUEST_SONG_LULLABY,  255, 255, 255),
    VANILLA_SONG_MAP_ENTRY(QUEST_SONG_EPONA,    255, 255, 255),
    VANILLA_SONG_MAP_ENTRY(QUEST_SONG_SARIA,    255, 255, 255),
    VANILLA_SONG_MAP_ENTRY(QUEST_SONG_SUN,      255, 255, 255),
    VANILLA_SONG_MAP_ENTRY(QUEST_SONG_TIME,     255, 255, 255),
    VANILLA_SONG_MAP_ENTRY(QUEST_SONG_STORMS,   255, 255, 255),
    VANILLA_SONG_MAP_ENTRY(QUEST_SONG_MINUET,   150, 255, 100),
    VANILLA_SONG_MAP_ENTRY(QUEST_SONG_BOLERO,   255, 80,  40),
    VANILLA_SONG_MAP_ENTRY(QUEST_SONG_SERENADE, 100, 150, 255),
    VANILLA_SONG_MAP_ENTRY(QUEST_SONG_REQUIEM,  255, 160, 0),
    VANILLA_SONG_MAP_ENTRY(QUEST_SONG_NOCTURNE, 255, 100, 255),
    VANILLA_SONG_MAP_ENTRY(QUEST_SONG_PRELUDE,  255, 240, 100),
} };

// Encapsulates what is drawn by the passed-in function within a border
template<typename T>
void DrawGroupWithBorder(T&& drawFunc) {
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
    ImGui::GetWindowDrawList()->AddRect(p0, p1, IM_COL32(borderCol.x * 255, borderCol.y * 255, borderCol.z * 255, borderCol.w * 255));

    ImGui::EndGroup();
}

char z2ASCII(int code) {
    int ret;
    if (code < 10) { //Digits
        ret = code + 0x30;
    } else if (code >= 10 && code < 36) { //Uppercase letters
        ret = code + 0x37;
    } else if (code >= 36 && code < 62) { //Lowercase letters
        ret = code + 0x3D;
    } else if (code == 62) { //Space
        ret = code - 0x1E;
    } else if (code == 63 || code == 64) { // _ and .
        ret = code - 0x12;
    } else {
        ret = code;
    }
    return char(ret);

}

void DrawInfoTab() {
    // TODO Needs a better method for name changing but for now this will work.
    std::string name;
    ImU16 one = 1;
    for (int i = 0; i < 8; i++) {
        char letter = z2ASCII(gSaveContext.playerName[i]);
        name += letter;
    }
    name += '\0';

    ImGui::PushItemWidth(ImGui::GetFontSize() * 6);

    ImGui::Text("Nombre: %s", name.c_str());
    UIWidgets::InsertHelpHoverText("Nombre del jugador");
    std::string nameID;
    for (int i = 0; i < 8; i++) {
        nameID = z2ASCII(i);
        if (i % 4 != 0) {
            ImGui::SameLine();
        }
        ImGui::InputScalar(nameID.c_str(), ImGuiDataType_U8, &gSaveContext.playerName[i], &one, NULL);
    }

    // Use an intermediary to keep the health from updating (and potentially killing the player)
    // until it is done being edited
    int16_t healthIntermediary = gSaveContext.healthCapacity;
    ImGui::InputScalar("Salud máxima", ImGuiDataType_S16, &healthIntermediary);
    if (ImGui::IsItemDeactivated()) {
        gSaveContext.healthCapacity = healthIntermediary;
    }
    UIWidgets::InsertHelpHoverText("Salud máxima. 16 unidades por corazón completo");
    if (gSaveContext.health > gSaveContext.healthCapacity) {
        gSaveContext.health = gSaveContext.healthCapacity; // Clamp health to new max
    }

    const uint16_t healthMin = 0;
    const uint16_t healthMax = gSaveContext.healthCapacity;
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 15);
    ImGui::SliderScalar("Salud", ImGuiDataType_S16, &gSaveContext.health, &healthMin, &healthMax);
    UIWidgets::InsertHelpHoverText("Salud actual. 16 unidades por corazón completo");

    bool isDoubleDefenseAcquired = gSaveContext.isDoubleDefenseAcquired != 0;
    if (ImGui::Checkbox("Doble defensa", &isDoubleDefenseAcquired)) {
        gSaveContext.isDoubleDefenseAcquired = isDoubleDefenseAcquired;
        gSaveContext.inventory.defenseHearts =
            gSaveContext.isDoubleDefenseAcquired ? 20 : 0; // Set to get the border drawn in the UI
    }
    UIWidgets::InsertHelpHoverText("¿Está desbloqueada la doble defensa?");

    std::string magicName;
    if (gSaveContext.magicLevel == 2) {
        magicName = "Doble";
    } else if (gSaveContext.magicLevel == 1) {
        magicName = "Simple";
    } else {
        magicName = "Ninguno";
    }
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6);
    if (ImGui::BeginCombo("Nivel de magia", magicName.c_str())) {
        if (ImGui::Selectable("Doble")) {
            gSaveContext.magicLevel = 2;
            gSaveContext.isMagicAcquired = true;
            gSaveContext.isDoubleMagicAcquired = true;
        }
        if (ImGui::Selectable("Simple")) {
            gSaveContext.magicLevel = 1;
            gSaveContext.isMagicAcquired = true;
            gSaveContext.isDoubleMagicAcquired = false;
        }
        if (ImGui::Selectable("Ninguno")) {
            gSaveContext.magicLevel = 0;
            gSaveContext.isMagicAcquired = false;
            gSaveContext.isDoubleMagicAcquired = false;
        }

        ImGui::EndCombo();
    }
    UIWidgets::InsertHelpHoverText("Nivel de magia actual");
    gSaveContext.magicCapacity = gSaveContext.magicLevel * 0x30; // Set to get the bar drawn in the UI
    if (gSaveContext.magic > gSaveContext.magicCapacity) {
        gSaveContext.magic = gSaveContext.magicCapacity; // Clamp magic to new max
    }

    const uint8_t magicMin = 0;
    const uint8_t magicMax = gSaveContext.magicCapacity;
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 15);
    ImGui::SliderScalar("Magia", ImGuiDataType_S8, &gSaveContext.magic, &magicMin, &magicMax);
    UIWidgets::InsertHelpHoverText("Magia actual. 48 unidades por nivel de magia");

    ImGui::InputScalar("Rupias", ImGuiDataType_S16, &gSaveContext.rupees);
    UIWidgets::InsertHelpHoverText("Rupias actuales");

    const uint16_t dayTimeMin = 0;
    const uint16_t dayTimeMax = 0xFFFF;
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 15);
    ImGui::SliderScalar("Hora", ImGuiDataType_U16, &gSaveContext.dayTime, &dayTimeMin, &dayTimeMax);
    UIWidgets::InsertHelpHoverText("Hora del día");
    if (ImGui::Button("Amanecer")) {
        gSaveContext.dayTime = 0x4000;
    }
    ImGui::SameLine();
    if (ImGui::Button("Mediodía")) {
        gSaveContext.dayTime = 0x8000;
    }
    ImGui::SameLine();
    if (ImGui::Button("Atardecer")) {
        gSaveContext.dayTime = 0xC001;
    }
    ImGui::SameLine();
    if (ImGui::Button("Medianoche")) {
        gSaveContext.dayTime = 0;
    }

    ImGui::InputScalar("Días totales", ImGuiDataType_S32, &gSaveContext.totalDays);
    UIWidgets::InsertHelpHoverText("Número total de días desde el inicio del juego");

    ImGui::InputScalar("Muertes", ImGuiDataType_U16, &gSaveContext.deaths);
    UIWidgets::InsertHelpHoverText("Número total de muertes");

    bool bgsFlag = gSaveContext.bgsFlag != 0;
    if (ImGui::Checkbox("Tiene Espada de Biggoron", &bgsFlag)) {
        gSaveContext.bgsFlag = bgsFlag;
    }
    UIWidgets::InsertHelpHoverText("¿Está desbloqueada la Espada de Biggoron? Sustituye a la Daga gigante");

    ImGui::InputScalar("Durabilidad de la espada", ImGuiDataType_U16, &gSaveContext.swordHealth);
    UIWidgets::InsertHelpHoverText("Durabilidad de la Daga gigante. Por defecto 8. Debe ser >0 para que funcione la Espada de Biggoron");

    ImGui::InputScalar("Días de la Espada de Biggoron", ImGuiDataType_S32, &gSaveContext.bgsDayCount);
    UIWidgets::InsertHelpHoverText("Número de días desde que se entregó el albarán a Biggoron");

    ImGui::InputScalar("Índice de entrada", ImGuiDataType_S32, &gSaveContext.entranceIndex);
    UIWidgets::InsertHelpHoverText("¿Por qué entrada llegó Link?");

    ImGui::InputScalar("Índice de escena", ImGuiDataType_S32, &gSaveContext.cutsceneIndex);
    UIWidgets::InsertHelpHoverText("¿Qué escena es?");

    ImGui::InputScalar("Temporizador de Navi", ImGuiDataType_U16, &gSaveContext.naviTimer);
    UIWidgets::InsertHelpHoverText("Navi quiere hablar a las 600 unidades y desiste a las 3000.");

    ImGui::InputScalar("Estado del temporizador 1", ImGuiDataType_S16, &gSaveContext.timer1State);
    UIWidgets::InsertHelpHoverText("Temporizador de calor, de carrera, etc. Texto blanco");

    ImGui::InputScalar("Valor del temporizador 1", ImGuiDataType_S16, &gSaveContext.timer1Value, &one, NULL);
    UIWidgets::InsertHelpHoverText("Tiempo, en segundos");

    ImGui::InputScalar("Estado del temporizador 2", ImGuiDataType_S16, &gSaveContext.timer2State);
    UIWidgets::InsertHelpHoverText("Temporizador de trueque, derrumbe de Ganon, etc. Texto amarillo");

    ImGui::InputScalar("Valor del temporizador 2", ImGuiDataType_S16, &gSaveContext.timer2Value, &one, NULL);
    UIWidgets::InsertHelpHoverText("Tiempo, en segundos");
     
    const char* audioName;
    switch (gSaveContext.audioSetting) { 
        case 0:
            audioName = "Estéreo";
            break;
        case 1:
            audioName = "Mono";
            break;
        case 2:
            audioName = "Auriculares";
            break;
        case 3:
            audioName = "Envolvente";
            break;
        default:
            audioName = "?";
    }
    if (ImGui::BeginCombo("Audio", audioName)) {
        if (ImGui::Selectable("Estéreo")) {
            gSaveContext.audioSetting = 0;
        }
        if (ImGui::Selectable("Mono")) {
            gSaveContext.audioSetting = 1;
        }
        if (ImGui::Selectable("Auriculares")) {
            gSaveContext.audioSetting = 2;
        }
        if (ImGui::Selectable("Envolvente")) {
            gSaveContext.audioSetting = 3;
        }

        ImGui::EndCombo();
    }
    UIWidgets::InsertHelpHoverText("Ajuste de sonido");
    
    bool n64DDFlag = gSaveContext.n64ddFlag != 0;
    if (ImGui::Checkbox("¿Partida 64DD?", &n64DDFlag)) {
        gSaveContext.n64ddFlag = n64DDFlag;
    }
    UIWidgets::InsertHelpHoverText("¡AVISO! Si guardas, la partida podría quedar bloqueada. ¡Cuidado!");
    
    if (ImGui::BeginCombo("Modo de apuntado Z", gSaveContext.zTargetSetting ? "Mantener" : "Alternar")) {
        if (ImGui::Selectable("Alternar")) {
            gSaveContext.zTargetSetting = 0;
        }
        if (ImGui::Selectable("Mantener")) {
            gSaveContext.zTargetSetting = 1;
        }
        ImGui::EndCombo();
    }
    UIWidgets::InsertHelpHoverText("Comportamiento del apuntado Z");


    ImGui::PushItemWidth(ImGui::GetFontSize() * 10);
    static std::array<const char*, 7> minigameHS = { "Arquería a caballo", 
        "Puntos de Poes grandes",                                            
        "Pesca",
        "Carrera de obstáculos de Malon",                                    
        "Carrera del corredor",
        "?",
        "Carrera de Dampé" };
    
    if (ImGui::TreeNode("Minijuegos")) {
        for (int i = 0; i < 7; i++) {
            if(i == 2 && ImGui::TreeNode("Pesca") ){ //fishing has a few more flags to it
                u8 fishSize = gSaveContext.highScores[i] & 0x7F;
                if(ImGui::InputScalar("Récord de tamaño (niño)",ImGuiDataType_U8,&fishSize)){
                    gSaveContext.highScores[i]&=~0x7F;
                    gSaveContext.highScores[i]|=fishSize & 0x7F;
                }
                char fishMsg[64];
                std::sprintf(fishMsg,"Peso: %2.0f lb",((SQ(fishSize)*.0036)+.5));
                UIWidgets::InsertHelpHoverText(fishMsg);
                bool FishBool = gSaveContext.highScores[i]&0x80;
                if (ImGui::Checkbox("Trampa como niño", &FishBool)) {
                        gSaveContext.highScores[i] &= ~0x80;
                        gSaveContext.highScores[i] |= (0x80 * FishBool);
                }
                UIWidgets::InsertHelpHoverText("Se usó el señuelo hundido para pescarlo.");
                fishSize=(gSaveContext.highScores[i] & 0x7F000000)>>0x18;
                if(ImGui::InputScalar("Récord de tamaño (adulto)",ImGuiDataType_U8,&fishSize)){
                    gSaveContext.highScores[i]&=~0x7F000000;
                    gSaveContext.highScores[i]|=(fishSize & 0x7F) << 0x18;
                }
                std::sprintf(fishMsg,"Peso: %2.0f lb",((SQ(fishSize)*.0036)+.5));
                UIWidgets::InsertHelpHoverText(fishMsg);
                FishBool = gSaveContext.highScores[i] & 0x80000000;
                if (ImGui::Checkbox("Trampa como adulto", &FishBool)) {
                        gSaveContext.highScores[i] &= ~0x80000000;
                        gSaveContext.highScores[i] |= (0x80000000 * FishBool);
                }
                UIWidgets::InsertHelpHoverText("Se usó el señuelo hundido para pescarlo.");
                FishBool = gSaveContext.highScores[i]&0x100;
                if (ImGui::Checkbox("Jugó como niño", &FishBool)) {
                        gSaveContext.highScores[i] &= ~0x100;
                        gSaveContext.highScores[i] |= (0x100 * FishBool);
                }
                UIWidgets::InsertHelpHoverText("Jugó al menos una partida como niño");
                FishBool = gSaveContext.highScores[i]&0x200;
                if (ImGui::Checkbox("Jugó como adulto", &FishBool)) {
                        gSaveContext.highScores[i] &= ~0x200;
                        gSaveContext.highScores[i] |= (0x200 * FishBool);
                }
                UIWidgets::InsertHelpHoverText("Jugó al menos una partida como adulto");
                FishBool = gSaveContext.highScores[i]&0x400;
                if (ImGui::Checkbox("Premio como niño", &FishBool)) {
                        gSaveContext.highScores[i] &= ~0x400;
                        gSaveContext.highScores[i] |= (0x400 * FishBool);
                }
                UIWidgets::InsertHelpHoverText("Obtuvo el premio (pieza de corazón, salvo en rando).\nDesbloquea el señuelo hundido para Link niño.");
                FishBool = gSaveContext.highScores[i]&0x800;
                if (ImGui::Checkbox("Premio como adulto", &FishBool)) {
                        gSaveContext.highScores[i] &= ~0x800;
                        gSaveContext.highScores[i] |= (0x800 * FishBool);
                }
                UIWidgets::InsertHelpHoverText("Obtuvo el premio (Escama de Oro, salvo en rando).\nDesbloquea el señuelo hundido para Link adulto.");
                FishBool = gSaveContext.highScores[i] & 0x1000;
                if (ImGui::Checkbox("Robó el sombrero del dueño", &FishBool)) {
                        gSaveContext.highScores[i] &= ~0x1000;
                        gSaveContext.highScores[i] |= (0x1000 * FishBool);
                }
                UIWidgets::InsertHelpHoverText("El dueño se ve calvo cuando Link es adulto.");
                fishSize=(gSaveContext.highScores[i] & 0xFF0000)>>16;
                if(ImGui::InputScalar("Veces jugadas",ImGuiDataType_U8,&fishSize)){
                    gSaveContext.highScores[i]&=~0xFF0000;
                    gSaveContext.highScores[i]|=(fishSize) << 16;
                }
                UIWidgets::InsertHelpHoverText("Determina el clima y el tamaño del banco de peces al amanecer/atardecer.");
                
                ImGui::TreePop();
                continue;
            }
            
            if (i == 5) { //HS_UNK_05 is unused
                continue;
            }
            std::string minigameLbl = minigameHS[i];
            ImGui::InputScalar(minigameLbl.c_str(), ImGuiDataType_S32, &gSaveContext.highScores[i], &one, NULL);
        }
        
        ImGui::TreePop();
    }
    
    ImGui::PopItemWidth();
}

void DrawBGSItemFlag(uint8_t itemID) {
    const ItemMapEntry& slotEntry = itemMapping[itemID];
    ImGui::Image(SohImGui::GetTextureByName(slotEntry.name), ImVec2(32.0f, 32.0f), ImVec2(0, 0), ImVec2(1, 1));
    ImGui::SameLine();
    int tradeIndex = itemID - ITEM_POCKET_EGG;
    bool hasItem = (gSaveContext.adultTradeItems & (1 << tradeIndex)) != 0;
    bool shouldHaveItem = hasItem;
    ImGui::Checkbox(("##adultTradeFlag" + std::to_string(itemID)).c_str(), &shouldHaveItem);
    if (hasItem != shouldHaveItem) {
        if (shouldHaveItem) {
            gSaveContext.adultTradeItems |= (1 << tradeIndex);
            if (INV_CONTENT(ITEM_TRADE_ADULT) == ITEM_NONE) {
                INV_CONTENT(ITEM_TRADE_ADULT) = ITEM_POCKET_EGG + tradeIndex;
            }
        } else {
            gSaveContext.adultTradeItems &= ~(1 << tradeIndex);
            Inventory_ReplaceItem(gPlayState, itemID, Randomizer_GetNextAdultTradeItem());
        }
    }
}

void DrawInventoryTab() {
    static bool restrictToValid = true;

    ImGui::Checkbox("Limitar a objetos válidos", &restrictToValid);
    UIWidgets::InsertHelpHoverText("Limita objetos y munición a lo que se puede obtener legalmente en el juego");

    for (int32_t y = 0; y < 4; y++) {
        for (int32_t x = 0; x < 6; x++) {
            int32_t index = x + y * 6;
            static int32_t selectedIndex = -1;
            static const char* itemPopupPicker = "itemPopupPicker";

            ImGui::PushID(index);

            if (x != 0) {
                ImGui::SameLine();
            }

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            uint8_t item = gSaveContext.inventory.items[index];
            if (item != ITEM_NONE) {
                const ItemMapEntry& slotEntry = itemMapping.find(item)->second;
                if (ImGui::ImageButton(SohImGui::GetTextureByName(slotEntry.name), ImVec2(32.0f, 32.0f), ImVec2(0, 0),
                                       ImVec2(1, 1), 0)) {
                    selectedIndex = index;
                    ImGui::OpenPopup(itemPopupPicker);
                }
            } else {
                if (ImGui::Button("##itemNone", ImVec2(32.0f, 32.0f))) {
                    selectedIndex = index;
                    ImGui::OpenPopup(itemPopupPicker);
                }
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
            if (ImGui::BeginPopup(itemPopupPicker)) {
                if (ImGui::Button("##itemNonePicker", ImVec2(32.0f, 32.0f))) {
                    gSaveContext.inventory.items[selectedIndex] = ITEM_NONE;
                    if (selectedIndex == SLOT_TRADE_ADULT) {
                        gSaveContext.adultTradeItems = 0;
                    }
                    ImGui::CloseCurrentPopup();
                }
                UIWidgets::SetLastItemHoverText("Ninguno");

                std::vector<ItemMapEntry> possibleItems;
                if (restrictToValid) {
                    // Scan gItemSlots to find legal items for this slot. Bottles are a special case
                    for (int slotIndex = 0; slotIndex < 56; slotIndex++) {
                        int testIndex = (selectedIndex == SLOT_BOTTLE_1 || selectedIndex == SLOT_BOTTLE_2 ||
                                         selectedIndex == SLOT_BOTTLE_3 || selectedIndex == SLOT_BOTTLE_4)
                                            ? SLOT_BOTTLE_1
                                            : selectedIndex;
                        if (gItemSlots[slotIndex] == testIndex) {
                            possibleItems.push_back(itemMapping[slotIndex]);
                        }
                    }
                } else {
                    for (const auto& entry : itemMapping) {
                        possibleItems.push_back(entry.second);
                    }
                }

                for (int32_t pickerIndex = 0; pickerIndex < possibleItems.size(); pickerIndex++) {
                    if (((pickerIndex + 1) % 8) != 0) {
                        ImGui::SameLine();
                    }
                    const ItemMapEntry& slotEntry = possibleItems[pickerIndex];
                    if (ImGui::ImageButton(SohImGui::GetTextureByName(slotEntry.name), ImVec2(32.0f, 32.0f),
                                           ImVec2(0, 0), ImVec2(1, 1), 0)) {
                        gSaveContext.inventory.items[selectedIndex] = slotEntry.id;
                        // Set adult trade item flag if you're playing adult trade shuffle in rando  
                        if (gSaveContext.n64ddFlag &&
                            OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_ADULT_TRADE) &&
                            selectedIndex == SLOT_TRADE_ADULT &&
                            slotEntry.id >= ITEM_POCKET_EGG && slotEntry.id <= ITEM_CLAIM_CHECK) {
                            gSaveContext.adultTradeItems |= ADULT_TRADE_FLAG(slotEntry.id);
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    UIWidgets::SetLastItemHoverText(SohUtils::GetItemName(slotEntry.id));
                }

                ImGui::EndPopup();
            }
            ImGui::PopStyleVar();

            ImGui::PopID();
        }
    }

    ImGui::Text("Munición");
    for (uint32_t ammoIndex = 0, drawnAmmoItems = 0; ammoIndex < 16; ammoIndex++) {
        uint8_t item = (restrictToValid) ? gAmmoItems[ammoIndex] : gAllAmmoItems[ammoIndex];
        if (item != ITEM_NONE) {
            // For legal items, display as 1 row of 7. For unrestricted items, display rows of 6 to match
            // inventory
            if ((restrictToValid && (drawnAmmoItems != 0)) || ((drawnAmmoItems % 6) != 0)) {
                ImGui::SameLine();
            }
            drawnAmmoItems++;

            ImGui::PushID(ammoIndex);
            ImGui::PushItemWidth(32.0f);
            ImGui::BeginGroup();

            ImGui::Image(SohImGui::GetTextureByName(itemMapping[item].name), ImVec2(32.0f, 32.0f));
            ImGui::InputScalar("##ammoInput", ImGuiDataType_S8, &AMMO(item));

            ImGui::EndGroup();
            ImGui::PopItemWidth();
            ImGui::PopID();
        }
    }
    
    // Trade quest flags are only used when shuffling the trade sequence, so
    // don't show this if it isn't needed.
    if (gSaveContext.n64ddFlag && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_ADULT_TRADE)
        && ImGui::TreeNode("Objetos del trueque de adulto")) {
        for (int i = ITEM_POCKET_EGG; i <= ITEM_CLAIM_CHECK; i++) {
            DrawBGSItemFlag(i);
        }
        ImGui::TreePop();
    }
}

// Draw a flag bitfield as an grid of checkboxes
void DrawFlagTableArray16(const FlagTable& flagTable, uint16_t row, uint16_t& flags) {
    ImGui::PushID((std::to_string(row) + flagTable.name).c_str());
    for (int32_t flagIndex = 15; flagIndex >= 0; flagIndex--) {
        ImGui::SameLine();
        ImGui::PushID(flagIndex);
        bool hasDescription = !!flagTable.flagDescriptions.contains(row * 16 + flagIndex);
        uint32_t bitMask = 1 << flagIndex;
        ImGui::PushStyleColor(ImGuiCol_FrameBg, hasDescription ? ImVec4(0.16f, 0.29f, 0.48f, 0.54f) : ImVec4(0.16f, 0.29f, 0.48f, 0.24f));
        bool flag = (flags & bitMask) != 0;
        if (ImGui::Checkbox("##check", &flag)) {
            if (flag) {
                flags |= bitMask;
            } else {
                flags &= ~bitMask;
            }
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered() && hasDescription) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", UIWidgets::WrappedText(flagTable.flagDescriptions.at(row * 16 + flagIndex), 60));
            ImGui::EndTooltip();
        }
        ImGui::PopID();
    }
    ImGui::PopID();
}

void DrawFlagsTab() {
    if (ImGui::TreeNode("Estado del jugador")) {
        if (gPlayState != nullptr) {
            Player* player = GET_PLAYER(gPlayState);

            DrawGroupWithBorder([&]() {
                ImGui::Text("stateFlags1");
                UIWidgets::DrawFlagArray32("stateFlags1", player->stateFlags1);
            });

            ImGui::SameLine();

            DrawGroupWithBorder([&]() {
                ImGui::Text("stateFlags2");
                UIWidgets::DrawFlagArray32("stateFlags2", player->stateFlags2);
            });

            DrawGroupWithBorder([&]() {
                ImGui::Text("stateFlags3");
                UIWidgets::DrawFlagArray8("stateFlags3", player->stateFlags3);
            });
            
            ImGui::SameLine();
            
            DrawGroupWithBorder([&]() {
                ImGui::Text("unk_6AE");
                UIWidgets::DrawFlagArray16("unk_6AE", player->unk_6AE);
            });
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Escena actual")) {
        if (gPlayState != nullptr) {
            ActorContext* act = &gPlayState->actorCtx;

            DrawGroupWithBorder([&]() {
                ImGui::Text("Interruptor");
                UIWidgets::InsertHelpHoverText("Flags de interruptores guardados de forma permanente");
                ImGui::SameLine();
                if (ImGui::Button("Marcar todo##Switch")) {
                    act->flags.swch = UINT32_MAX;
                }
                ImGui::SameLine();
                if (ImGui::Button("Borrar todo##Switch")) {
                    act->flags.swch = 0;
                }
                UIWidgets::DrawFlagArray32("Interruptor", act->flags.swch);
            });

            ImGui::SameLine();

            DrawGroupWithBorder([&]() {
                ImGui::Text("Interruptor temp.");
                UIWidgets::InsertHelpHoverText("Flags de interruptores temporales. Se borran al cambiar de escena");
                ImGui::SameLine();
                if (ImGui::Button("Marcar todo##Temp Switch")) {
                    act->flags.tempSwch = UINT32_MAX;
                }
                ImGui::SameLine();
                if (ImGui::Button("Borrar todo##Temp Switch")) {
                    act->flags.tempSwch = 0;
                }
                UIWidgets::DrawFlagArray32("Interruptor temp.", act->flags.tempSwch);
            });

            DrawGroupWithBorder([&]() {
                ImGui::Text("Sala despejada");
                UIWidgets::InsertHelpHoverText("Flags de sala despejada guardados de forma permanente");
                ImGui::SameLine();
                if (ImGui::Button("Marcar todo##Clear")) {
                    act->flags.clear = UINT32_MAX;
                }
                ImGui::SameLine();
                if (ImGui::Button("Borrar todo##Clear")) {
                    act->flags.clear = 0;
                }
                UIWidgets::DrawFlagArray32("Sala despejada", act->flags.clear);
            });

            ImGui::SameLine();

            DrawGroupWithBorder([&]() {
                ImGui::Text("Sala despejada temp.");
                UIWidgets::InsertHelpHoverText("Flags de sala despejada temporales. Se borran al cambiar de escena");
                ImGui::SameLine();
                if (ImGui::Button("Marcar todo##Temp Clear")) {
                    act->flags.tempClear = UINT32_MAX;
                }
                ImGui::SameLine();
                if (ImGui::Button("Borrar todo##Temp Clear")) {
                    act->flags.tempClear = 0;
                }
                UIWidgets::DrawFlagArray32("Sala despejada temp.", act->flags.tempClear);
            });

            DrawGroupWithBorder([&]() {
                ImGui::Text("Recogidos");
                UIWidgets::InsertHelpHoverText("Flags de objetos recogidos guardados de forma permanente");
                ImGui::SameLine();
                if (ImGui::Button("Marcar todo##Collect")) {
                    act->flags.collect = UINT32_MAX;
                }
                ImGui::SameLine();
                if (ImGui::Button("Borrar todo##Collect")) {
                    act->flags.collect = 0;
                }
                UIWidgets::DrawFlagArray32("Recogidos", act->flags.collect);
            });

            ImGui::SameLine();

            DrawGroupWithBorder([&]() {
                ImGui::Text("Recogidos temp.");
                UIWidgets::InsertHelpHoverText("Flags de objetos recogidos temporales. Se borran al cambiar de escena");
                ImGui::SameLine();
                if (ImGui::Button("Marcar todo##Temp Collect")) {
                    act->flags.tempCollect = UINT32_MAX;
                }
                ImGui::SameLine();
                if (ImGui::Button("Borrar todo##Temp Collect")) {
                    act->flags.tempCollect = 0;
                }
                UIWidgets::DrawFlagArray32("Recogidos temp.", act->flags.tempCollect);
            });

            DrawGroupWithBorder([&]() {
                ImGui::Text("Cofre");
                UIWidgets::InsertHelpHoverText("Flags de cofres guardados de forma permanente");
                ImGui::SameLine();
                if (ImGui::Button("Marcar todo##Chest")) {
                    act->flags.chest = UINT32_MAX;
                }
                ImGui::SameLine();
                if (ImGui::Button("Borrar todo##Chest")) {
                    act->flags.chest = 0;
                }
                UIWidgets::DrawFlagArray32("Cofre", act->flags.chest);
            });

            ImGui::SameLine();

            ImGui::BeginGroup();

            if (ImGui::Button("Recargar flags")) {
                act->flags.swch = gSaveContext.sceneFlags[gPlayState->sceneNum].swch;
                act->flags.clear = gSaveContext.sceneFlags[gPlayState->sceneNum].clear;
                act->flags.collect = gSaveContext.sceneFlags[gPlayState->sceneNum].collect;
                act->flags.chest = gSaveContext.sceneFlags[gPlayState->sceneNum].chest;
            }
            UIWidgets::SetLastItemHoverText("Carga los flags guardados de la escena. Normalmente ocurre al cargar la escena");

            if (ImGui::Button("Guardar flags")) {
                gSaveContext.sceneFlags[gPlayState->sceneNum].swch = act->flags.swch;
                gSaveContext.sceneFlags[gPlayState->sceneNum].clear = act->flags.clear;
                gSaveContext.sceneFlags[gPlayState->sceneNum].collect = act->flags.collect;
                gSaveContext.sceneFlags[gPlayState->sceneNum].chest = act->flags.chest;
            }
            UIWidgets::SetLastItemHoverText("Guarda los flags de la escena actual. Normalmente ocurre al salir de la escena");
            
            if (ImGui::Button("Borrar flags")) {
                act->flags.swch = 0;
                act->flags.clear = 0;
                act->flags.collect = 0;
                act->flags.chest = 0;
            }
            UIWidgets::SetLastItemHoverText("Borra los flags de la escena actual. Recarga la escena para ver los cambios");


            ImGui::EndGroup();
        } else {
            ImGui::Text("El estado actual del juego no tiene una escena activa");
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Flags de escenas guardadas")) {
        static uint32_t selectedSceneFlagMap = 0;
        ImGui::Text("Mapa");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##Map", SohUtils::GetSceneName(selectedSceneFlagMap).c_str())) {
            for (int32_t sceneIndex = 0; sceneIndex < SCENE_ID_MAX; sceneIndex++) {
                if (ImGui::Selectable(SohUtils::GetSceneName(sceneIndex).c_str())) {
                    selectedSceneFlagMap = sceneIndex;
                }
            }

            ImGui::EndCombo();
        }

        // Don't show current scene button if there is no current scene
        if (gPlayState != nullptr) {
            ImGui::SameLine();
            if (ImGui::Button("Actual")) {
                selectedSceneFlagMap = gPlayState->sceneNum;
            }
            UIWidgets::SetLastItemHoverText("Abre los flags de la escena actual");
        }

        DrawGroupWithBorder([&]() {
            ImGui::Text("Interruptor");
            UIWidgets::InsertHelpHoverText("Flags de interruptores");
            UIWidgets::DrawFlagArray32("Interruptor", gSaveContext.sceneFlags[selectedSceneFlagMap].swch);
        });

        ImGui::SameLine();

        DrawGroupWithBorder([&]() {
            ImGui::Text("Sala despejada");
            UIWidgets::InsertHelpHoverText("Flags de sala despejada");
            UIWidgets::DrawFlagArray32("Sala despejada", gSaveContext.sceneFlags[selectedSceneFlagMap].clear);
        });

        DrawGroupWithBorder([&]() {
            ImGui::Text("Recogidos");
            UIWidgets::InsertHelpHoverText("Flags de objetos recogidos");
            UIWidgets::DrawFlagArray32("Recogidos", gSaveContext.sceneFlags[selectedSceneFlagMap].collect);
        });

        ImGui::SameLine();

        DrawGroupWithBorder([&]() {
            ImGui::Text("Cofre");
            UIWidgets::InsertHelpHoverText("Flags de cofres");
            UIWidgets::DrawFlagArray32("Cofre", gSaveContext.sceneFlags[selectedSceneFlagMap].chest);
        });

        DrawGroupWithBorder([&]() {
            ImGui::Text("Salas");
            UIWidgets::InsertHelpHoverText("Flags de salas visitadas");
            UIWidgets::DrawFlagArray32("Salas", gSaveContext.sceneFlags[selectedSceneFlagMap].rooms);
        });

        ImGui::SameLine();

        DrawGroupWithBorder([&]() {
            ImGui::Text("Pisos");
            UIWidgets::InsertHelpHoverText("Flags de pisos visitados");
            UIWidgets::DrawFlagArray32("Pisos", gSaveContext.sceneFlags[selectedSceneFlagMap].floors);
        });

        ImGui::TreePop();
    }

    DrawGroupWithBorder([&]() {
        static uint32_t selectedGsMap = 0;
        ImGui::Text("Skulltulas doradas");
        ImGui::Text("Mapa");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##Gold Skulltula Map", gsMapping[selectedGsMap].c_str())) {
            for (int32_t gsIndex = 0; gsIndex < gsMapping.size(); gsIndex++) {
                if (ImGui::Selectable(gsMapping[gsIndex].c_str())) {
                    selectedGsMap = gsIndex;
                }
            }

            ImGui::EndCombo();
        }

        // TODO We should write out descriptions for each one... ugh
        ImGui::Text("Flags");
        uint32_t currentFlags = GET_GS_FLAGS(selectedGsMap);
        uint32_t allFlags = gAreaGsFlags[selectedGsMap];
        uint32_t setMask = 1;
        // Iterate over bitfield and create a checkbox for each skulltula
        while (allFlags != 0) {
            bool isThisSet = (currentFlags & 0x1) == 0x1;

            ImGui::SameLine();
            ImGui::PushID(allFlags);
            if (ImGui::Checkbox("##gs", &isThisSet)) {
                if (isThisSet) {
                    SET_GS_FLAGS(selectedGsMap, setMask);
                } else {
                    // Have to do this roundabout method as the macro does not support clearing flags
                    uint32_t currentFlagsBase = GET_GS_FLAGS(selectedGsMap);
                    gSaveContext.gsFlags[selectedGsMap >> 2] &= ~gGsFlagsMasks[selectedGsMap & 3];
                    SET_GS_FLAGS(selectedGsMap, currentFlagsBase & ~setMask);
                }
            }

            ImGui::PopID();

            allFlags >>= 1;
            currentFlags >>= 1;
            setMask <<= 1;
        }

        // If playing a Randomizer Save with Shuffle Skull Tokens on anything other than "Off" we don't want to keep
        // GS Token Count updated, since Gold Skulltulas killed will not correlate to GS Tokens Collected.
        if (!(gSaveContext.n64ddFlag && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_TOKENS) != RO_TOKENSANITY_OFF)) {
            static bool keepGsCountUpdated = true;
            ImGui::Checkbox("Mantener actualizada la cuenta de Skulltulas", &keepGsCountUpdated);
            UIWidgets::InsertHelpHoverText("Ajusta automáticamente el número de símbolos de skulltula dorada según los flags marcados.");
            int32_t gsCount = 0;
            if (keepGsCountUpdated) {
                for (int32_t gsFlagIndex = 0; gsFlagIndex < 6; gsFlagIndex++) {
                    gsCount += std::popcount(static_cast<uint32_t>(gSaveContext.gsFlags[gsFlagIndex]));
                }
                gSaveContext.inventory.gsTokens = gsCount;
            }
        }
    });

    for (int i = 0; i < flagTables.size(); i++) {
        const FlagTable& flagTable = flagTables[i];
        if (flagTable.flagTableType == RANDOMIZER_INF && !gSaveContext.n64ddFlag) {
            continue;
        }

        if (ImGui::TreeNode(flagTable.name)) {
            for (int j = 0; j < flagTable.size + 1; j++) {
                DrawGroupWithBorder([&]() {
                    ImGui::Text(fmt::format("{:<2x}", j).c_str());
                    switch (flagTable.flagTableType) {
                        case EVENT_CHECK_INF:
                            DrawFlagTableArray16(flagTable, j, gSaveContext.eventChkInf[j]);
                            break;
                        case ITEM_GET_INF:
                            DrawFlagTableArray16(flagTable, j, gSaveContext.itemGetInf[j]);
                            break;
                        case INF_TABLE:
                            DrawFlagTableArray16(flagTable, j, gSaveContext.infTable[j]);
                            break;
                        case EVENT_INF:
                            DrawFlagTableArray16(flagTable, j, gSaveContext.eventInf[j]);
                            break;
                        case RANDOMIZER_INF:
                            DrawFlagTableArray16(flagTable, j, gSaveContext.randomizerInf[j]);
                            break;
                    }
                });
            }
            ImGui::TreePop();
        }
    }
}

// Draws a combo that lets you choose and upgrade value from a drop-down of text values
void DrawUpgrade(const std::string& categoryName, int32_t categoryId, const std::vector<std::string>& names) {
    ImGui::Text("%s", categoryName.c_str());
    ImGui::SameLine();
    ImGui::PushID(categoryName.c_str());
    if (ImGui::BeginCombo("##upgrade", names[CUR_UPG_VALUE(categoryId)].c_str())) {
        for (int32_t i = 0; i < names.size(); i++) {
            if (ImGui::Selectable(names[i].c_str())) {
                Inventory_ChangeUpgrade(categoryId, i);
            }
        }

        ImGui::EndCombo();
    }
    ImGui::PopID();
    UIWidgets::SetLastItemHoverText(categoryName.c_str());
}

// Draws a combo that lets you choose and upgrade value from a popup grid of icons
void DrawUpgradeIcon(const std::string& categoryName, int32_t categoryId, const std::vector<uint8_t>& items) {
    static const char* upgradePopupPicker = "upgradePopupPicker";

    ImGui::PushID(categoryName.c_str());

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    uint8_t item = items[CUR_UPG_VALUE(categoryId)];
    if (item != ITEM_NONE) {
        const ItemMapEntry& slotEntry = itemMapping[item];
        if (ImGui::ImageButton(SohImGui::GetTextureByName(slotEntry.name), ImVec2(32.0f, 32.0f), ImVec2(0, 0),
                               ImVec2(1, 1), 0)) {
            ImGui::OpenPopup(upgradePopupPicker);
        }
    } else {
        if (ImGui::Button("##itemNone", ImVec2(32.0f, 32.0f))) {
            ImGui::OpenPopup(upgradePopupPicker);
        }
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    UIWidgets::SetLastItemHoverText(categoryName.c_str());

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    if (ImGui::BeginPopup(upgradePopupPicker)) {
        for (int32_t pickerIndex = 0; pickerIndex < items.size(); pickerIndex++) {
            if ((pickerIndex % 8) != 0) {
                ImGui::SameLine();
            }

            if (items[pickerIndex] == ITEM_NONE) {
                if (ImGui::Button("##upgradePopupPicker", ImVec2(32.0f, 32.0f))) {
                    Inventory_ChangeUpgrade(categoryId, pickerIndex);
                    ImGui::CloseCurrentPopup();
                }
                UIWidgets::SetLastItemHoverText("Ninguno");
            } else {
                const ItemMapEntry& slotEntry = itemMapping[items[pickerIndex]];
                if (ImGui::ImageButton(SohImGui::GetTextureByName(slotEntry.name), ImVec2(32.0f, 32.0f), ImVec2(0, 0),
                                       ImVec2(1, 1), 0)) {
                    Inventory_ChangeUpgrade(categoryId, pickerIndex);
                    ImGui::CloseCurrentPopup();
                }
                UIWidgets::SetLastItemHoverText(SohUtils::GetItemName(slotEntry.id));
            }
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    ImGui::PopID();
}

void DrawEquipmentTab() {
    const std::vector<uint8_t> equipmentValues = {
        ITEM_SWORD_KOKIRI, ITEM_SWORD_MASTER,  ITEM_SWORD_BGS,     ITEM_SWORD_BROKEN,
        ITEM_SHIELD_DEKU,  ITEM_SHIELD_HYLIAN, ITEM_SHIELD_MIRROR, ITEM_NONE,
        ITEM_TUNIC_KOKIRI, ITEM_TUNIC_GORON,   ITEM_TUNIC_ZORA,    ITEM_NONE,
        ITEM_BOOTS_KOKIRI, ITEM_BOOTS_IRON,    ITEM_BOOTS_HOVER,   ITEM_NONE,
    };
    for (int32_t i = 0; i < equipmentValues.size(); i++) {
        // Skip over unused 4th slots for shields, boots, and tunics
        if (equipmentValues[i] == ITEM_NONE) {
            continue;
        }
        if ((i % 4) != 0) {
            ImGui::SameLine();
        }

        ImGui::PushID(i);
        uint32_t bitMask = 1 << i;
        bool hasEquip = (bitMask & gSaveContext.inventory.equipment) != 0;
        const ItemMapEntry& entry = itemMapping[equipmentValues[i]];
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        if (ImGui::ImageButton(SohImGui::GetTextureByName(hasEquip ? entry.name : entry.nameFaded),
                               ImVec2(32.0f, 32.0f), ImVec2(0, 0), ImVec2(1, 1), 0)) {
            if (hasEquip) {
                gSaveContext.inventory.equipment &= ~bitMask;
            } else {
                gSaveContext.inventory.equipment |= bitMask;
            }
        }
        ImGui::PopStyleColor();
        ImGui::PopID();
        UIWidgets::SetLastItemHoverText(SohUtils::GetItemName(entry.id));
    }

    const std::vector<uint8_t> bulletBagValues = {
        ITEM_NONE,
        ITEM_BULLET_BAG_30,
        ITEM_BULLET_BAG_40,
        ITEM_BULLET_BAG_50,
    };
    DrawUpgradeIcon("Bolsa de semillas deku", UPG_BULLET_BAG, bulletBagValues);

    ImGui::SameLine();

    const std::vector<uint8_t> quiverValues = {
        ITEM_NONE,
        ITEM_QUIVER_30,
        ITEM_QUIVER_40,
        ITEM_QUIVER_50,
    };
    DrawUpgradeIcon("Carcaj", UPG_QUIVER, quiverValues);

    ImGui::SameLine();

    const std::vector<uint8_t> bombBagValues = {
        ITEM_NONE,
        ITEM_BOMB_BAG_20,
        ITEM_BOMB_BAG_30,
        ITEM_BOMB_BAG_40,
    };
    DrawUpgradeIcon("Saco de bombas", UPG_BOMB_BAG, bombBagValues);

    ImGui::SameLine();

    const std::vector<uint8_t> scaleValues = {
        ITEM_NONE,
        ITEM_SCALE_SILVER,
        ITEM_SCALE_GOLDEN,
    };
    DrawUpgradeIcon("Escama", UPG_SCALE, scaleValues);

    ImGui::SameLine();

    const std::vector<uint8_t> strengthValues = {
        ITEM_NONE,
        ITEM_BRACELET,
        ITEM_GAUNTLETS_SILVER,
        ITEM_GAUNTLETS_GOLD,
    };
    DrawUpgradeIcon("Fuerza", UPG_STRENGTH, strengthValues);

    // There is no icon for child wallet, so default to a text list
    // this was const, but I needed to append to it depending in rando settings.
    std::vector<std::string> walletNamesImpl = {
        "Niño (99)",
        "Adulto (200)",
        "Gigante (500)",
    };
    // only display Tycoon wallet if you're in a save file that would allow it.
    if (gSaveContext.n64ddFlag && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHOPSANITY) > RO_SHOPSANITY_ZERO_ITEMS) {
        const std::string walletName = "Magnate (999)";
        walletNamesImpl.push_back(walletName);
    }
    // copy it to const value for display in ImGui.
    const std::vector<std::string> walletNames = walletNamesImpl;
    DrawUpgrade("Bolsa", UPG_WALLET, walletNames);

    const std::vector<std::string> stickNames = {
        "Ninguno",
        "10",
        "20",
        "30",
    };
    DrawUpgrade("Palos deku", UPG_STICKS, stickNames);

    const std::vector<std::string> nutNames = {
        "Ninguno",
        "20",
        "30",
        "40",
    };
    DrawUpgrade("Nueces deku", UPG_NUTS, nutNames);
}

// Draws a toggleable icon for a quest item that is faded when disabled
void DrawQuestItemButton(uint32_t item) {
    const QuestMapEntry& entry = questMapping[item];
    uint32_t bitMask = 1 << entry.id;
    bool hasQuestItem = (bitMask & gSaveContext.inventory.questItems) != 0;
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::ImageButton(SohImGui::GetTextureByName(hasQuestItem ? entry.name : entry.nameFaded),
                           ImVec2(32.0f, 32.0f), ImVec2(0, 0), ImVec2(1, 1), 0)) {
        if (hasQuestItem) {
            gSaveContext.inventory.questItems &= ~bitMask;
        } else {
            gSaveContext.inventory.questItems |= bitMask;
        }
    }
    ImGui::PopStyleColor();
    UIWidgets::SetLastItemHoverText(SohUtils::GetQuestItemName(entry.id));
}

// Draws a toggleable icon for a dungeon item that is faded when disabled
void DrawDungeonItemButton(uint32_t item, uint32_t scene) {
    const ItemMapEntry& entry = itemMapping[item];
    uint32_t bitMask = 1 << (entry.id - ITEM_KEY_BOSS); // Bitset starts at ITEM_KEY_BOSS == 0. the rest are sequential
    bool hasItem = (bitMask & gSaveContext.inventory.dungeonItems[scene]) != 0;
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::ImageButton(SohImGui::GetTextureByName(hasItem ? entry.name : entry.nameFaded),
                           ImVec2(32.0f, 32.0f), ImVec2(0, 0), ImVec2(1, 1), 0)) {
        if (hasItem) {
            gSaveContext.inventory.dungeonItems[scene] &= ~bitMask;
        } else {
            gSaveContext.inventory.dungeonItems[scene] |= bitMask;
        }
    }
    ImGui::PopStyleColor();
    UIWidgets::SetLastItemHoverText(SohUtils::GetItemName(entry.id));
}

void DrawQuestStatusTab() {
    ImGui::PushItemWidth(ImGui::GetFontSize() * 6);

    for (int32_t i = QUEST_MEDALLION_FOREST; i < QUEST_MEDALLION_LIGHT + 1; i++) {
        if (i != QUEST_MEDALLION_FOREST) {
            ImGui::SameLine();
        }
        DrawQuestItemButton(i);
    }

    for (int32_t i = QUEST_KOKIRI_EMERALD; i < QUEST_ZORA_SAPPHIRE + 1; i++) {
        if (i != QUEST_KOKIRI_EMERALD) {
            ImGui::SameLine();
        }
        DrawQuestItemButton(i);
    }

    // Put Stone of Agony and Gerudo Card on the same line with a little space between them
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(20, 0));

    ImGui::SameLine();
    DrawQuestItemButton(QUEST_STONE_OF_AGONY);

    ImGui::SameLine();
    DrawQuestItemButton(QUEST_GERUDO_CARD);

    for (const SongMapEntry& entry : songMapping) {
        if ((entry.id != QUEST_SONG_MINUET) && (entry.id != QUEST_SONG_LULLABY)) {
            ImGui::SameLine();
        }

        uint32_t bitMask = 1 << entry.id;
        bool hasQuestItem = (bitMask & gSaveContext.inventory.questItems) != 0;
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        if (ImGui::ImageButton(SohImGui::GetTextureByName(hasQuestItem ? entry.name : entry.nameFaded),
                               ImVec2(16.0f, 24.0f), ImVec2(0, 0), ImVec2(1, 1), 0)) {
            if (hasQuestItem) {
                gSaveContext.inventory.questItems &= ~bitMask;
            } else {
                gSaveContext.inventory.questItems |= bitMask;
            }
        }
        ImGui::PopStyleColor();
        UIWidgets::SetLastItemHoverText(SohUtils::GetQuestItemName(entry.id));
    }

    ImGui::InputScalar("Cuenta de Skulltulas", ImGuiDataType_S16, &gSaveContext.inventory.gsTokens);
    UIWidgets::InsertHelpHoverText("Número de símbolos de skulltula dorada obtenidos");

    uint32_t bitMask = 1 << QUEST_SKULL_TOKEN;
    bool gsUnlocked = (bitMask & gSaveContext.inventory.questItems) != 0;
    if (ImGui::Checkbox("Skulltulas desbloqueadas", &gsUnlocked)) {
        if (gsUnlocked) {
            gSaveContext.inventory.questItems |= bitMask;
        } else {
            gSaveContext.inventory.questItems &= ~bitMask;
        }
    }
    UIWidgets::InsertHelpHoverText("Si está desbloqueado, muestra la cuenta de skulltulas doradas en el menú de misión");

    int32_t pohCount = (gSaveContext.inventory.questItems & 0xF0000000) >> 28;
    if (ImGui::BeginCombo("Piezas de corazón", std::to_string(pohCount).c_str())) {
        for (int32_t i = 0; i < 4; i++) {
            if (ImGui::Selectable(std::to_string(i).c_str(), pohCount == i)) {
                gSaveContext.inventory.questItems &= ~0xF0000000;
                gSaveContext.inventory.questItems |= (i << 28);
            }
        }
        ImGui::EndCombo();
    }
    UIWidgets::InsertHelpHoverText("Número de piezas de corazón hacia el siguiente contenedor");

    DrawGroupWithBorder([&]() {
        ImGui::Text("Objetos de mazmorra");

        static int32_t dungeonItemsScene = SCENE_YDAN;
        ImGui::PushItemWidth(-ImGui::GetWindowWidth() * 0.35f);
        if (ImGui::BeginCombo("##DungeonSelect", SohUtils::GetSceneName(dungeonItemsScene).c_str())) {
            for (int32_t dungeonIndex = SCENE_YDAN; dungeonIndex < SCENE_BDAN_BOSS + 1; dungeonIndex++) {
                if (ImGui::Selectable(SohUtils::GetSceneName(dungeonIndex).c_str(),
                                      dungeonIndex == dungeonItemsScene)) {
                    dungeonItemsScene = dungeonIndex;
                }
            }

            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        DrawDungeonItemButton(ITEM_KEY_BOSS, dungeonItemsScene);
        ImGui::SameLine();
        DrawDungeonItemButton(ITEM_COMPASS, dungeonItemsScene);
        ImGui::SameLine();
        DrawDungeonItemButton(ITEM_DUNGEON_MAP, dungeonItemsScene);

        if (dungeonItemsScene != SCENE_BDAN_BOSS) {
            float lineHeight = ImGui::GetTextLineHeightWithSpacing();
            ImGui::Image(SohImGui::GetTextureByName(itemMapping[ITEM_KEY_SMALL].name), ImVec2(lineHeight, lineHeight));
            ImGui::SameLine();
            if (ImGui::InputScalar("##Keys", ImGuiDataType_S8, gSaveContext.inventory.dungeonKeys + dungeonItemsScene)) {
                gSaveContext.sohStats.dungeonKeys[dungeonItemsScene] = gSaveContext.inventory.dungeonKeys[dungeonItemsScene];
            };
        } else {
            // dungeonItems is size 20 but dungeonKeys is size 19, so there are no keys for the last scene (Barinade's Lair)
            ImGui::Text("La guarida de Barinade no tiene llaves pequeñas");
        }
    });

    ImGui::PopItemWidth();
}

void DrawPlayerTab() {
    if (gPlayState != nullptr) {
        Player* player = GET_PLAYER(gPlayState);
        const char* curSword;
        const char* curShield;
        const char* curTunic;
        const char* curBoots;

        switch (player->currentSwordItemId) {
            case ITEM_SWORD_KOKIRI:
                curSword = "Espada Kokiri"; 
                break;
            case ITEM_SWORD_MASTER:
                curSword = "Espada Maestra";
                break;
            case ITEM_SWORD_BGS:
                curSword = "Espada de Biggoron";
                break;
            case ITEM_FISHING_POLE:
                curSword = "Caña de pescar";
                break;
            case ITEM_NONE:
                curSword = "Ninguno";
                break;
            default:
                curSword = "Ninguno";
                break;
        }

        switch (player->currentShield) {
            case PLAYER_SHIELD_NONE:
                curShield = "Ninguno";
                break;
            case PLAYER_SHIELD_DEKU:
                curShield = "Escudo deku";
                break;
            case PLAYER_SHIELD_HYLIAN:
                curShield = "Escudo hyliano";
                break;
            case PLAYER_SHIELD_MIRROR:
                curShield = "Escudo espejo";
                break;
            default:
                break;
        }

        switch (player->currentTunic) {
            case PLAYER_TUNIC_KOKIRI:
                curTunic = "Sayo kokiri";
                break;
            case PLAYER_TUNIC_GORON:
                curTunic = "Sayo goron";
                break;
            case PLAYER_TUNIC_ZORA:
                curTunic = "Sayo zora";
                break;
            default:
                break;
        }

        switch (player->currentBoots) {
            case PLAYER_BOOTS_KOKIRI:
                curBoots = "Botas kokiri";
                break;
            case PLAYER_BOOTS_IRON:
                curBoots = "Botas de hierro";
                break;
            case PLAYER_BOOTS_HOVER:
                curBoots = "Botas voladoras";
                break;
            default:
                break;
        }

        ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
        DrawGroupWithBorder([&]() {
            ImGui::Text("Posición de Link");
            ImGui::InputScalar("Pos. X", ImGuiDataType_Float, &player->actor.world.pos.x);
            ImGui::SameLine();
            ImGui::InputScalar("Pos. Y", ImGuiDataType_Float, &player->actor.world.pos.y);
            ImGui::SameLine();
            ImGui::InputScalar("Pos. Z", ImGuiDataType_Float, &player->actor.world.pos.z);
        });

        DrawGroupWithBorder([&]() {
            ImGui::Text("Rotación de Link");
            UIWidgets::InsertHelpHoverText("Rotación de Link respecto al mundo");
            ImGui::InputScalar("Rot. X", ImGuiDataType_S16, &player->actor.world.rot.x);
            ImGui::SameLine();
            ImGui::InputScalar("Rot. Y", ImGuiDataType_S16, &player->actor.world.rot.y);
            ImGui::SameLine();
            ImGui::InputScalar("Rot. Z", ImGuiDataType_S16, &player->actor.world.rot.z);
        });

        DrawGroupWithBorder([&]() {
            ImGui::Text("Rotación del modelo de Link");
            UIWidgets::InsertHelpHoverText("Para el modelo real de Link");
            ImGui::InputScalar("Rot. modelo X", ImGuiDataType_S16, &player->actor.shape.rot.x);
            ImGui::SameLine();
            ImGui::InputScalar("Rot. modelo Y", ImGuiDataType_S16, &player->actor.shape.rot.y);
            ImGui::SameLine();
            ImGui::InputScalar("Rot. modelo Z", ImGuiDataType_S16, &player->actor.shape.rot.z);
        });

        ImGui::InputScalar("Velocidad lineal", ImGuiDataType_Float, &player->linearVelocity);
        UIWidgets::InsertHelpHoverText("Velocidad de Link en el plano XZ");

        ImGui::InputScalar("Velocidad Y", ImGuiDataType_Float, &player->actor.velocity.y);
        UIWidgets::InsertHelpHoverText("Velocidad de Link en el eje Y. Límite en -20");

        ImGui::InputScalar("Altura de pared", ImGuiDataType_Float, &player->wallHeight);
        UIWidgets::InsertHelpHoverText("Altura que determina si Link puede trepar o agarrarse a un borde");

        ImGui::InputScalar("Temporizador de invencibilidad", ImGuiDataType_S8, &player->invincibilityTimer);
        UIWidgets::InsertHelpHoverText("No recibe daño mientras no sea cero");

        ImGui::InputScalar("Gravedad", ImGuiDataType_Float, &player->actor.gravity);
        UIWidgets::InsertHelpHoverText("Velocidad de caída de Link. Por defecto -4.0f");

        if (ImGui::BeginCombo("Edad de Link al cargar", gPlayState->linkAgeOnLoad == 0 ? "Adulto" : "Niño")) {
            if (ImGui::Selectable("Adulto")) {
                gPlayState->linkAgeOnLoad = 0;
            }
            if (ImGui::Selectable("Niño")) {
                gPlayState->linkAgeOnLoad = 1;
            }
            ImGui::EndCombo();
        }

        UIWidgets::InsertHelpHoverText("Cambia la edad de Link al cargar un mapa");

        ImGui::Separator();
        
        ImGui::Text("Equipo actual de Link");
        ImGui::PushItemWidth(ImGui::GetFontSize() * 15);
        if (ImGui::BeginCombo("Espada", curSword)) {
            if (ImGui::Selectable("Ninguno")) {
                player->currentSwordItemId = ITEM_NONE;
                gSaveContext.equips.buttonItems[0] = ITEM_NONE;
                Inventory_ChangeEquipment(EQUIP_SWORD, PLAYER_SWORD_NONE);
            }
            if (ImGui::Selectable("Espada Kokiri")) {
                player->currentSwordItemId = ITEM_SWORD_KOKIRI;
                gSaveContext.equips.buttonItems[0] = ITEM_SWORD_KOKIRI;
                Inventory_ChangeEquipment(EQUIP_SWORD, PLAYER_SWORD_KOKIRI);
            }
            if (ImGui::Selectable("Espada Maestra")) {
                player->currentSwordItemId = ITEM_SWORD_MASTER;
                gSaveContext.equips.buttonItems[0] = ITEM_SWORD_MASTER;
                Inventory_ChangeEquipment(EQUIP_SWORD, PLAYER_SWORD_MASTER);
            }
            if (ImGui::Selectable("Espada de Biggoron")) {
                if (gSaveContext.bgsFlag) {
                    if (gSaveContext.swordHealth < 8) {
                        gSaveContext.swordHealth = 8;
                    }
                    player->currentSwordItemId = ITEM_SWORD_BGS;
                    gSaveContext.equips.buttonItems[0] = ITEM_SWORD_BGS;
                } else {
                    if (gSaveContext.swordHealth < 8) {
                        gSaveContext.swordHealth = 8;
                    }
                    player->currentSwordItemId = ITEM_SWORD_BGS;
                    gSaveContext.equips.buttonItems[0] = ITEM_SWORD_KNIFE;
                }
                
                Inventory_ChangeEquipment(EQUIP_SWORD, PLAYER_SWORD_BGS);
            }
            if (ImGui::Selectable("Caña de pescar")) {
                player->currentSwordItemId = ITEM_FISHING_POLE;
                gSaveContext.equips.buttonItems[0] = ITEM_FISHING_POLE;
                Inventory_ChangeEquipment(EQUIP_SWORD, PLAYER_SWORD_MASTER);
            }
            ImGui::EndCombo();

        }
        if (ImGui::BeginCombo("Escudo", curShield)) {
            if (ImGui::Selectable("Ninguno")) {
                player->currentShield = PLAYER_SHIELD_NONE;
                Inventory_ChangeEquipment(EQUIP_SHIELD, PLAYER_SHIELD_NONE);
            }
            if (ImGui::Selectable("Escudo deku")) {
                player->currentShield = PLAYER_SHIELD_DEKU;
                Inventory_ChangeEquipment(EQUIP_SHIELD, PLAYER_SHIELD_DEKU);
            }
            if (ImGui::Selectable("Escudo hyliano")) {
                player->currentShield = PLAYER_SHIELD_HYLIAN;
                Inventory_ChangeEquipment(EQUIP_SHIELD, PLAYER_SHIELD_HYLIAN);
            }
            if (ImGui::Selectable("Escudo espejo")) {
                player->currentShield = PLAYER_SHIELD_MIRROR;
                Inventory_ChangeEquipment(EQUIP_SHIELD, PLAYER_SHIELD_MIRROR);
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Sayo", curTunic)) {
            if (ImGui::Selectable("Sayo kokiri")) {
                player->currentTunic = PLAYER_TUNIC_KOKIRI;
                Inventory_ChangeEquipment(EQUIP_TUNIC, PLAYER_TUNIC_KOKIRI + 1);
            }
            if (ImGui::Selectable("Sayo goron")) {
                player->currentTunic = PLAYER_TUNIC_GORON;
                Inventory_ChangeEquipment(EQUIP_TUNIC, PLAYER_TUNIC_GORON + 1);
            }
            if (ImGui::Selectable("Sayo zora")) {
                player->currentTunic = PLAYER_TUNIC_ZORA;
                Inventory_ChangeEquipment(EQUIP_TUNIC, PLAYER_TUNIC_ZORA + 1);
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Botas", curBoots)) {
            if (ImGui::Selectable("Botas kokiri")) {
                player->currentBoots = PLAYER_BOOTS_KOKIRI;
                Inventory_ChangeEquipment(EQUIP_BOOTS, PLAYER_BOOTS_KOKIRI + 1);
            }
            if (ImGui::Selectable("Botas de hierro")) {
                player->currentBoots = PLAYER_BOOTS_IRON;
                Inventory_ChangeEquipment(EQUIP_BOOTS, PLAYER_BOOTS_IRON + 1);
            }
            if (ImGui::Selectable("Botas voladoras")) {
                player->currentBoots = PLAYER_BOOTS_HOVER;
                Inventory_ChangeEquipment(EQUIP_BOOTS, PLAYER_BOOTS_HOVER + 1);
            }
            ImGui::EndCombo();
        }

        ImU16 one = 1;
        ImGui::PushItemWidth(ImGui::GetFontSize() * 6);
        DrawGroupWithBorder([&]() {
            ImGui::Text("Equipo C actual");
            ImGui::InputScalar("C izquierda", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[1], &one, NULL);
            ImGui::SameLine();
            ImGui::InputScalar("C abajo", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[2], &one, NULL);
            ImGui::SameLine();
            ImGui::InputScalar("C derecha", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[3], &one, NULL);

            if (CVarGetInteger("gDpadEquips", 0)) {
                ImGui::NewLine();
                ImGui::Text("Equipo de la cruceta actual");
                ImGui::InputScalar("Cruceta arriba  ", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[4], &one, NULL); // Two spaces at the end for aligning, not elegant but it's working
                ImGui::SameLine();
                ImGui::InputScalar("Cruceta abajo", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[5], &one, NULL);
                // Intentionnal to not put everything on the same line, else it's taking too much for lower resolution.
                ImGui::InputScalar("Cruceta izquierda", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[6], &one, NULL);
                ImGui::SameLine();
                ImGui::InputScalar("Cruceta derecha", ImGuiDataType_U8, &gSaveContext.equips.buttonItems[7], &one, NULL);
            }
        });

        ImGui::Text("Estado del jugador");
        uint8_t bit[32] = {};
        uint32_t flags[3] = { player->stateFlags1, player->stateFlags2, player->stateFlags3 };

        for (int j = 0; j <= 2; j++) {
            DrawGroupWithBorder([&]() {
                ImGui::Text("Flags de estado %d", j + 1);
                for (int i = 0; i <= 31; i++) {
                    bit[i] = ((flags[j] >> i) & 1);
                    if (bit[i] != 0) {
                        ImGui::Text("Flag %d", i);
                    }
                }
            });
            ImGui::SameLine();
        }
        DrawGroupWithBorder([&]() {
            ImGui::Text("Espada");
            ImGui::Text("  %d", player->swordState);
        });

    } else {
        ImGui::Text("¡Se necesita el contexto global para la información del jugador!");
    }
}

void DrawSaveEditor(bool& open) {
    if (!open) {
        CVarSetInteger("gSaveEditorEnabled", 0);
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Editor de partida", &open, ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("SaveContextTabBar", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)) {
        if (ImGui::BeginTabItem("Info")) {
            DrawInfoTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Inventario")) {
            DrawInventoryTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Flags")) {
            DrawFlagsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Equipo")) {
            DrawEquipmentTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Estado de misión")) {
            DrawQuestStatusTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Jugador")) {
            DrawPlayerTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void InitSaveEditor() {
    SohImGui::AddWindow("Developer Tools", "Save Editor", DrawSaveEditor);

    // Load item icons into ImGui
    for (const auto& entry : itemMapping) {
        SohImGui::LoadResource(entry.second.name, entry.second.texturePath);
        SohImGui::LoadResource(entry.second.nameFaded, entry.second.texturePath, ImVec4(1, 1, 1, 0.3f));
    }
    for (const auto& entry : gregMapping) {
        ImVec4 gregGreen = ImVec4(42.0f / 255.0f, 169.0f / 255.0f, 40.0f / 255.0f, 1.0f);
        ImVec4 gregFadedGreen = gregGreen;
        gregFadedGreen.w = 0.3f;
        SohImGui::LoadResource(entry.second.name, entry.second.texturePath, gregGreen);
        SohImGui::LoadResource(entry.second.nameFaded, entry.second.texturePath, gregFadedGreen);
    }
    for (const auto& entry : questMapping) {
        SohImGui::LoadResource(entry.second.name, entry.second.texturePath);
        SohImGui::LoadResource(entry.second.nameFaded, entry.second.texturePath, ImVec4(1, 1, 1, 0.3f));
    }
    for (const auto& entry : songMapping) {
        SohImGui::LoadResource(entry.name, gSongNoteTex, entry.color);
        ImVec4 fadedCol = entry.color;
        fadedCol.w = 0.3f;
        SohImGui::LoadResource(entry.nameFaded, gSongNoteTex, fadedCol);
    }
    for (const auto& entry : vanillaSongMapping) {
        SohImGui::LoadResource(entry.name, gSongNoteTex, entry.color);
        ImVec4 fadedCol = entry.color;
        fadedCol.w = 0.3f;
        SohImGui::LoadResource(entry.nameFaded, gSongNoteTex, fadedCol);
    }
}
