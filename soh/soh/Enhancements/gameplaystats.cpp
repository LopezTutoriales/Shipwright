#include "gameplaystats.h"

#include "ImGuiImpl.h"
#include "../UIWidgets.hpp"

#include <map>
#include <string>
#include <libultraship/bridge.h>
#include <Hooks.h>

extern "C" {
#include <z64.h>
#include "variables.h"
}

#define COLOR_WHITE      ImVec4(1.00f, 1.00f, 1.00f, 1.00f)
#define COLOR_RED        ImVec4(1.00f, 0.00f, 0.00f, 1.00f)
#define COLOR_GREEN      ImVec4(0.10f, 1.00f, 0.10f, 1.00f)
#define COLOR_BLUE       ImVec4(0.00f, 0.33f, 1.00f, 1.00f)
#define COLOR_PURPLE     ImVec4(0.54f, 0.19f, 0.89f, 1.00f)
#define COLOR_YELLOW     ImVec4(1.00f, 1.00f, 0.00f, 1.00f)
#define COLOR_ORANGE     ImVec4(1.00f, 0.67f, 0.11f, 1.00f)
#define COLOR_LIGHT_BLUE ImVec4(0.00f, 0.88f, 1.00f, 1.00f)
#define COLOR_GREY       ImVec4(0.78f, 0.78f, 0.78f, 1.00f)

char timestampDisplayName[TIMESTAMP_MAX][32] = { "" };
ImVec4 timestampDisplayColor[TIMESTAMP_MAX];

typedef struct {
    char name[32];
    u32 time;
    ImVec4 color;
}TimestampInfo;

// Timestamps are an array of structs, each with a name, time, and color
// Names and colors are set up at the bottom of this file
// Times are stored in gSaveContext.sohStats.timestamp
TimestampInfo timestampDisplay[TIMESTAMP_MAX];

void DisplayTimeHHMMSS(uint32_t timeInTenthsOfSeconds, const char* text, ImVec4 color) {

    uint32_t sec = timeInTenthsOfSeconds / 10;
    uint32_t hh = sec / 3600;
    uint32_t mm = (sec - hh * 3600) / 60;
    uint32_t ss = sec - hh * 3600 - mm * 60;
    uint32_t ds = timeInTenthsOfSeconds % 10;

    ImGui::PushStyleColor(ImGuiCol_Text, color);

    ImGui::Text(text);
    ImGui::SameLine();
    ImGui::Text("%2u:%02u:%02u.%u", hh, mm, ss, ds);
    ImGui::PopStyleColor();
}

void SortChronological(TimestampInfo* arr, size_t len) {
    TimestampInfo temp;
    for (int i = 0; i < len; i++) {
        for (int j = 0; j + 1 < len - i; j++) {
            if (arr[j].time > arr[j + 1].time) {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

void DisplayStat(const char* text, uint32_t value) {

    ImGui::Text(text);
    ImGui::SameLine();
    ImGui::Text("%7u", value);
}

void DisplayStatIfNonZero(const char* text, uint32_t value) {
    if (value > 0) {
        DisplayStat(text, value);
    }
    return;
}

void DrawStatsTracker(bool& open) {
    if (!open) {
        CVarSetInteger("gGameplayStatsEnabled", 0);
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(480, 550), ImGuiCond_Appearing);
    if (!ImGui::Begin("Estadísticas de juego", &open, ImGuiWindowFlags_NoFocusOnAppearing)) {
        ImGui::End();
        return;
    }
    
    bool showTimestamps = (CVarGetInteger("gGameplayStatsMode", 0) <= 1);
    bool showCounts = ( (CVarGetInteger("gGameplayStatsMode", 0) == 0) || (CVarGetInteger("gGameplayStatsMode", 0) == 2) );

    u32 totalTimer = GAMEPLAYSTAT_TOTAL_TIME;
    u32 enemiesDefeated = 0;
    u32 ammoUsed = 0;
    u32 buttonPresses = 0;

    // Sum of all enemies defeated
    for (int i = COUNT_ENEMIES_DEFEATED_ANUBIS; i <= COUNT_ENEMIES_DEFEATED_WOLFOS; i++) {
        if (i == COUNT_ENEMIES_DEFEATED_FLOORMASTER) {
            // Special case: You must kill 3 mini Floormasters for it count as one defeated Floormaster
            enemiesDefeated += gSaveContext.sohStats.count[i] / 3;
        } else {
            enemiesDefeated += gSaveContext.sohStats.count[i];
        }
    }
    // Sum of all ammo used
    for (int i = COUNT_AMMO_USED_STICK; i <= COUNT_AMMO_USED_BEAN; i++) {
        ammoUsed += gSaveContext.sohStats.count[i];
    }
    // Sum of all button presses
    for (int i = COUNT_BUTTON_PRESSES_A; i <= COUNT_BUTTON_PRESSES_START; i++) {
        buttonPresses += gSaveContext.sohStats.count[i];
    }
    // Set up the array of timestamps and then sort it chronologically
    for (int i = 0; i < TIMESTAMP_MAX; i++) {
        strcpy(timestampDisplay[i].name, timestampDisplayName[i]);
        timestampDisplay[i].time = gSaveContext.sohStats.timestamp[i];
        timestampDisplay[i].color = timestampDisplayColor[i];
    }
    SortChronological(timestampDisplay, sizeof(timestampDisplay) / sizeof(timestampDisplay[0]));

    
    // Begin drawing the table and showing the stats

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 8.0f, 8.0f });
    ImGui::BeginTable("timers", 1, ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV);
    ImGui::TableSetupColumn("Tiempos", ImGuiTableColumnFlags_WidthStretch, 200.0f);
    ImGui::TableNextColumn();

    DisplayTimeHHMMSS(totalTimer, "Tiempo total:       ", COLOR_WHITE);
    UIWidgets::Tooltip("La precisión del cronómetro puede verse afectada por el rendimiento del juego y las cargas.");
    DisplayTimeHHMMSS(gSaveContext.sohStats.playTimer / 2, "Tiempo de juego:    ", COLOR_WHITE);
    UIWidgets::Tooltip("La precisión del cronómetro puede verse afectada por el rendimiento del juego y las cargas.");
    DisplayTimeHHMMSS(gSaveContext.sohStats.pauseTimer / 3, "Tiempo en pausa:    ", COLOR_WHITE);

    ImGui::PopStyleVar(1);
    ImGui::EndTable();

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 8.0f, 8.0f });
    ImGui::BeginTable("gameStatsTable", (showTimestamps && showCounts) ? 2 : 1, ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV);

    if (showTimestamps) {
        ImGui::TableSetupColumn("Hitos", ImGuiTableColumnFlags_WidthStretch, 200.0f);
    }
    if (showCounts) {
        ImGui::TableSetupColumn("Contadores", ImGuiTableColumnFlags_WidthStretch, 200.0f);
    }
    ImGui::TableHeadersRow();
    ImGui::TableNextRow();

    if (showTimestamps) {
        ImGui::TableNextColumn();
        
        // Display chronological timestamps of items obtained and bosses defeated
        for (int i = 0; i < TIMESTAMP_MAX; i++) {
            // To be shown, the entry must have a non-zero time and a string for its display name
            if (timestampDisplay[i].time > 0 && strnlen(timestampDisplay[i].name, 32) > 1) {
                DisplayTimeHHMMSS(timestampDisplay[i].time, timestampDisplay[i].name, timestampDisplay[i].color);
            }
        }
    }

    if (showCounts) {
        ImGui::TableNextColumn();

        DisplayStat("Enemigos vencidos:     ", enemiesDefeated);
        // Show breakdown of enemies defeated in a tree. Only show counts for enemies if they've been defeated at least once.
        if (enemiesDefeated > 0) {
            if (ImGui::TreeNode("Detalles enemigos...")) {

                DisplayStatIfNonZero("Anubis:             ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_ANUBIS]);
                DisplayStatIfNonZero("Armos:              ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_ARMOS]);
                DisplayStatIfNonZero("Arwing:             ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_ARWING]);
                DisplayStatIfNonZero("Bari:               ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_BARI]);
                DisplayStatIfNonZero("Biri:               ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_BIRI]);
                DisplayStatIfNonZero("Beamos:             ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_BEAMOS]);
                DisplayStatIfNonZero("Octo grande:        ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_BIG_OCTO]);
                DisplayStatIfNonZero("Burbuja (azul):     ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_BUBBLE_BLUE]);
                DisplayStatIfNonZero("Burbuja (verde):    ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_BUBBLE_GREEN]);
                DisplayStatIfNonZero("Burbuja (roja):     ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_BUBBLE_RED]);
                DisplayStatIfNonZero("Burbuja (blanca):   ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_BUBBLE_WHITE]);
                DisplayStatIfNonZero("Deku mercader:      ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_BUSINESS_SCRUB]);
                DisplayStatIfNonZero("Link Oscuro:        ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_DARK_LINK]);
                DisplayStatIfNonZero("Mano muerta:        ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_DEAD_HAND]);
                DisplayStatIfNonZero("Deku Baba:          ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_DEKU_BABA]);
                DisplayStatIfNonZero("Deku Baba (grande): ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_DEKU_BABA_BIG]);
                DisplayStatIfNonZero("Deku:               ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_DEKU_SCRUB]);
                DisplayStatIfNonZero("Dinolfos:           ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_DINOLFOS]);
                DisplayStatIfNonZero("Dodongo:            ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_DODONGO]);
                DisplayStatIfNonZero("Dodongo (cría):     ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_DODONGO_BABY]);
                DisplayStatIfNonZero("Puerta falsa:       ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_DOOR_TRAP]);
                DisplayStatIfNonZero("Bailarín de fuego:  ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_FLARE_DANCER]);
                DisplayStatIfNonZero("Floormaster:        ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_FLOORMASTER]/3);
                DisplayStatIfNonZero("Loseta voladora:    ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_FLOOR_TILE]);
                DisplayStatIfNonZero("Vasija voladora:    ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_FLYING_POT]);
                DisplayStatIfNonZero("Freezard:           ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_FREEZARD]);
                DisplayStatIfNonZero("Ladrona gerudo:     ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_GERUDO_THIEF]);
                DisplayStatIfNonZero("Gibdo:              ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_GIBDO]);
                DisplayStatIfNonZero("Larva de Gohma:     ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_GOHMA_LARVA]);
                DisplayStatIfNonZero("Guay:               ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_GUAY]);
                DisplayStatIfNonZero("Iron Knuckle:       ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_IRON_KNUCKLE]);
                DisplayStatIfNonZero("Iron Knuckle (Nab): ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_IRON_KNUCKLE_NABOORU]);
                DisplayStatIfNonZero("Keese:              ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_KEESE]);
                DisplayStatIfNonZero("Keese (fuego):      ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_KEESE_FIRE]);
                DisplayStatIfNonZero("Keese (hielo):      ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_KEESE_ICE]);
                DisplayStatIfNonZero("Leever:             ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_LEEVER]);
                DisplayStatIfNonZero("Leever (grande):    ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_LEEVER_BIG]);
                DisplayStatIfNonZero("Like-Like:          ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_LIKE_LIKE]);
                DisplayStatIfNonZero("Lizalfos:           ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_LIZALFOS]);
                DisplayStatIfNonZero("Deku furioso:       ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_MAD_SCRUB]);
                DisplayStatIfNonZero("Moblin:             ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_MOBLIN]);
                DisplayStatIfNonZero("Moblin (maza):      ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_MOBLIN_CLUB]);
                DisplayStatIfNonZero("Octorok:            ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_OCTOROK]);
                DisplayStatIfNonZero("Tentáculo parásito: ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_PARASITIC_TENTACLE]);
                DisplayStatIfNonZero("Peahat:             ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_PEAHAT]);
                DisplayStatIfNonZero("Larva de Peahat:    ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_PEAHAT_LARVA]);
                DisplayStatIfNonZero("Poe:                ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_POE]);
                DisplayStatIfNonZero("Poe (grande):       ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_POE_BIG]);
                DisplayStatIfNonZero("Poe (compositor):   ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_POE_COMPOSER]);
                DisplayStatIfNonZero("Hermanas Poe:       ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_POE_SISTERS]);
                DisplayStatIfNonZero("Redead:             ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_REDEAD]);
                DisplayStatIfNonZero("Shabom:             ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_SHABOM]);
                DisplayStatIfNonZero("Shellblade:         ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_SHELLBLADE]);
                DisplayStatIfNonZero("Skull Kid:          ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_SKULL_KID]);
                DisplayStatIfNonZero("Skulltula:          ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_SKULLTULA]);
                DisplayStatIfNonZero("Skulltula (grande): ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_SKULLTULA_BIG]);
                DisplayStatIfNonZero("Skulltula (dorada): ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_SKULLTULA_GOLD]);
                DisplayStatIfNonZero("Skullwalltula:      ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_SKULLWALLTULA]);
                DisplayStatIfNonZero("Spike:              ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_SPIKE]);
                DisplayStatIfNonZero("Stalchild:          ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_STALCHILD]);
                DisplayStatIfNonZero("Stalfos:            ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_STALFOS]);
                DisplayStatIfNonZero("Stinger:            ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_STINGER]);
                DisplayStatIfNonZero("Tailpasaran:        ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_TAILPASARAN]);
                DisplayStatIfNonZero("Tektite (azul):     ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_TEKTITE_BLUE]);
                DisplayStatIfNonZero("Tektite (rojo):     ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_TEKTITE_RED]);
                DisplayStatIfNonZero("Babosa antorcha:    ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_TORCH_SLUG]);
                DisplayStatIfNonZero("Wallmaster:         ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_WALLMASTER]);
                DisplayStatIfNonZero("Deku Baba marchita: ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_WITHERED_DEKU_BABA]);
                DisplayStatIfNonZero("Wolfos:             ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_WOLFOS]);
                DisplayStatIfNonZero("Wolfos (blanco):    ", gSaveContext.sohStats.count[COUNT_ENEMIES_DEFEATED_WOLFOS_WHITE]);
            
            ImGui::NewLine();
            ImGui::TreePop();
            }
        }
        
        DisplayStat("Rupias recogidas:      ", gSaveContext.sohStats.count[COUNT_RUPEES_COLLECTED]);
        UIWidgets::Tooltip("Incluye las rupias recogidas con la bolsa llena.");
        DisplayStat("Rupias gastadas:       ", gSaveContext.sohStats.count[COUNT_RUPEES_SPENT]);
        DisplayStat("Cofres abiertos:       ", gSaveContext.sohStats.count[COUNT_CHESTS_OPENED]);

        DisplayStat("Munición usada:        ", ammoUsed);
        // Show breakdown of ammo used in a collapsible tree. Only show ammo types if they've been used at least once.
        if (ammoUsed > 0) {
            if (ImGui::TreeNode("Detalles munición...")) {

                DisplayStatIfNonZero("Palos deku:         ", gSaveContext.sohStats.count[COUNT_AMMO_USED_STICK]);
                DisplayStatIfNonZero("Nueces deku:        ", gSaveContext.sohStats.count[COUNT_AMMO_USED_NUT]);
                DisplayStatIfNonZero("Semillas deku:      ", gSaveContext.sohStats.count[COUNT_AMMO_USED_SEED]);
                DisplayStatIfNonZero("Bombas:             ", gSaveContext.sohStats.count[COUNT_AMMO_USED_BOMB]);
                DisplayStatIfNonZero("Bombuchus:          ", gSaveContext.sohStats.count[COUNT_AMMO_USED_BOMBCHU]);
                DisplayStatIfNonZero("Flechas:            ", gSaveContext.sohStats.count[COUNT_AMMO_USED_ARROW]);
                DisplayStatIfNonZero("Judías:             ", gSaveContext.sohStats.count[COUNT_AMMO_USED_BEAN]);

                ImGui::NewLine();
                ImGui::TreePop();
            }
        }

        DisplayStat("Daño recibido:         ", gSaveContext.sohStats.count[COUNT_DAMAGE_TAKEN]);
        DisplayStat("Tajos de espada:       ", gSaveContext.sohStats.count[COUNT_SWORD_SWINGS]);
        DisplayStat("Pasos dados:           ", gSaveContext.sohStats.count[COUNT_STEPS]);
        // If using MM Bunny Hood enhancement, show how long it's been equipped (not counting pause time)
        if (CVarGetInteger("gMMBunnyHood", 0) || gSaveContext.sohStats.count[COUNT_TIME_BUNNY_HOOD] > 0) {
            DisplayTimeHHMMSS(gSaveContext.sohStats.count[COUNT_TIME_BUNNY_HOOD] / 2, "Tiempo capucha:     ", COLOR_WHITE);
        }
        DisplayStat("Volteretas:            ", gSaveContext.sohStats.count[COUNT_ROLLS]);
        DisplayStat("Topetazos:             ", gSaveContext.sohStats.count[COUNT_BONKS]);
        DisplayStat("Saltos laterales:      ", gSaveContext.sohStats.count[COUNT_SIDEHOPS]);
        DisplayStat("Saltos atrás:          ", gSaveContext.sohStats.count[COUNT_BACKFLIPS]);
        DisplayStat("Trampas de hielo:      ", gSaveContext.sohStats.count[COUNT_ICE_TRAPS]);
        DisplayStat("Pausas:                ", gSaveContext.sohStats.count[COUNT_PAUSES]);
        DisplayStat("Vasijas rotas:         ", gSaveContext.sohStats.count[COUNT_POTS_BROKEN]);
        DisplayStat("Arbustos cortados:     ", gSaveContext.sohStats.count[COUNT_BUSHES_CUT]);

        DisplayStat("Botones pulsados:      ", buttonPresses);
        // Show breakdown of ammo used in a collapsible tree. Only show ammo types if they've been used at least once.
        if (buttonPresses > 0) {
            if (ImGui::TreeNode("Botones...")) {

                DisplayStatIfNonZero("A:                  ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_A]);
                DisplayStatIfNonZero("B:                  ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_B]);
                DisplayStatIfNonZero("L:                  ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_L]);
                DisplayStatIfNonZero("R:                  ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_R]);
                DisplayStatIfNonZero("Z:                  ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_Z]);
                DisplayStatIfNonZero("C arriba:           ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_CUP]);
                DisplayStatIfNonZero("C derecha:          ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_CRIGHT]);
                DisplayStatIfNonZero("C abajo:            ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_CDOWN]);
                DisplayStatIfNonZero("C izquierda:        ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_CLEFT]);
                DisplayStatIfNonZero("Cruceta arriba:     ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_DUP]);
                DisplayStatIfNonZero("Cruceta derecha:    ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_DRIGHT]);
                DisplayStatIfNonZero("Cruceta abajo:      ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_DDOWN]);
                DisplayStatIfNonZero("Cruceta izquierda:  ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_DLEFT]);
                DisplayStatIfNonZero("Start:              ", gSaveContext.sohStats.count[COUNT_BUTTON_PRESSES_START]);

                ImGui::NewLine();
                ImGui::TreePop();
            }
        }
    }

    ImGui::PopStyleVar(1);
    ImGui::EndTable();

    const char* gameplayStatsModeOptions[3] = { "Ambos", "Hitos", "Contadores"};
    
    ImGui::Text("Modo de vista");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
    UIWidgets::EnhancementCombobox("gGameplayStatsMode", gameplayStatsModeOptions, 3, 0);

    ImGui::Text("Nota: las estadísticas se guardan en el archivo actual y\nse perderán si sales sin guardar.");

    ImGui::End();
}

// Entries listed here will have a timestamp shown in the stat window
void SetupDisplayNames() {
    // To add a timestamp for an item or event, add it to this list and ensure
    // it has a corresponding entry in the enum (see gameplaystats.h)

    strcpy(timestampDisplayName[ITEM_BOW],              "Arco de las Hadas:  ");
    strcpy(timestampDisplayName[ITEM_ARROW_FIRE],       "Flechas de fuego:   ");
    strcpy(timestampDisplayName[ITEM_DINS_FIRE],        "Fuego de Din:       ");
    strcpy(timestampDisplayName[ITEM_SLINGSHOT],        "Tirachinas:         ");
    strcpy(timestampDisplayName[ITEM_OCARINA_FAIRY],    "Ocarina de Hadas:   ");
    strcpy(timestampDisplayName[ITEM_OCARINA_TIME],     "Ocarina del Tiempo: ");
    strcpy(timestampDisplayName[ITEM_BOMBCHU],          "Bombuchus:          ");
    strcpy(timestampDisplayName[ITEM_HOOKSHOT],         "Gancho:             ");
    strcpy(timestampDisplayName[ITEM_LONGSHOT],         "Supergancho:        ");
    strcpy(timestampDisplayName[ITEM_ARROW_ICE],        "Flechas de hielo:   ");
    strcpy(timestampDisplayName[ITEM_FARORES_WIND],     "Viento de Farore:   ");
    strcpy(timestampDisplayName[ITEM_BOOMERANG],        "Bumerán:            ");
    strcpy(timestampDisplayName[ITEM_LENS],             "Lupa de la Verdad:  ");
    strcpy(timestampDisplayName[ITEM_HAMMER],           "Martillo Megatón:   ");
    strcpy(timestampDisplayName[ITEM_ARROW_LIGHT],      "Flechas de luz:     ");
    strcpy(timestampDisplayName[ITEM_BOTTLE],           "Botella:            ");
    strcpy(timestampDisplayName[ITEM_LETTER_ZELDA],     "Carta de Zelda:     ");
    strcpy(timestampDisplayName[ITEM_SWORD_KOKIRI],     "Espada Kokiri:      ");
    strcpy(timestampDisplayName[ITEM_SWORD_MASTER],     "Espada Maestra:     ");
    strcpy(timestampDisplayName[ITEM_SWORD_BGS],        "Espada Biggoron:    ");
    strcpy(timestampDisplayName[ITEM_SHIELD_DEKU],      "Escudo deku:        ");
    strcpy(timestampDisplayName[ITEM_SHIELD_HYLIAN],    "Escudo hyliano:     ");
    strcpy(timestampDisplayName[ITEM_SHIELD_MIRROR],    "Escudo espejo:      ");
    strcpy(timestampDisplayName[ITEM_TUNIC_GORON],      "Sayo goron:         ");
    strcpy(timestampDisplayName[ITEM_TUNIC_ZORA],       "Sayo zora:          ");
    strcpy(timestampDisplayName[ITEM_BOOTS_IRON],       "Botas de hierro:    ");
    strcpy(timestampDisplayName[ITEM_BOOTS_HOVER],      "Botas voladoras:    ");
    strcpy(timestampDisplayName[ITEM_BOMB_BAG_20],      "Saco de bombas:     ");
    strcpy(timestampDisplayName[ITEM_BRACELET],         "Brazalete Goron:    ");
    strcpy(timestampDisplayName[ITEM_GAUNTLETS_SILVER], "Guantes de Plata:   ");
    strcpy(timestampDisplayName[ITEM_GAUNTLETS_GOLD],   "Guantes de Oro:     ");
    strcpy(timestampDisplayName[ITEM_SCALE_SILVER],     "Escama de Plata:    ");
    strcpy(timestampDisplayName[ITEM_SCALE_GOLDEN],     "Escama de Oro:      ");
    strcpy(timestampDisplayName[ITEM_WALLET_ADULT],     "Bolsa de adulto:    ");
    strcpy(timestampDisplayName[ITEM_WALLET_GIANT],     "Bolsa gigante:      ");
    strcpy(timestampDisplayName[ITEM_WEIRD_EGG],        "Huevo extraño:      ");
    strcpy(timestampDisplayName[ITEM_GERUDO_CARD],      "Pase gerudo:        ");
    strcpy(timestampDisplayName[ITEM_COJIRO],           "Cojiro:             ");
    strcpy(timestampDisplayName[ITEM_POCKET_EGG],       "Huevo de bolsillo:  ");
    strcpy(timestampDisplayName[ITEM_MASK_SKULL],       "Máscara calavera:   ");
    strcpy(timestampDisplayName[ITEM_MASK_SPOOKY],      "Máscara tenebrosa:  ");
    strcpy(timestampDisplayName[ITEM_MASK_KEATON],      "Careta de Keaton:   ");
    strcpy(timestampDisplayName[ITEM_MASK_BUNNY],       "Capucha de conejo:  ");
    strcpy(timestampDisplayName[ITEM_ODD_MUSHROOM],     "Seta rara:          ");
    strcpy(timestampDisplayName[ITEM_ODD_POTION],       "Medicina rara:      ");
    strcpy(timestampDisplayName[ITEM_SAW],              "Sierra del furtivo: ");
    strcpy(timestampDisplayName[ITEM_SWORD_BROKEN],     "Espada goron rota:  ");
    strcpy(timestampDisplayName[ITEM_PRESCRIPTION],     "Receta:             ");
    strcpy(timestampDisplayName[ITEM_FROG],             "Rana ojos saltones: ");
    strcpy(timestampDisplayName[ITEM_EYEDROPS],         "Supergotas:         ");
    strcpy(timestampDisplayName[ITEM_CLAIM_CHECK],      "Albarán:            ");
    strcpy(timestampDisplayName[ITEM_SONG_MINUET],      "Minueto del bosque: ");
    strcpy(timestampDisplayName[ITEM_SONG_BOLERO],      "Bolero del fuego:   ");
    strcpy(timestampDisplayName[ITEM_SONG_SERENADE],    "Serenata del agua:  ");
    strcpy(timestampDisplayName[ITEM_SONG_REQUIEM],     "Réquiem espíritu:   ");
    strcpy(timestampDisplayName[ITEM_SONG_NOCTURNE],    "Nocturno sombra:    ");
    strcpy(timestampDisplayName[ITEM_SONG_PRELUDE],     "Preludio de la luz: ");
    strcpy(timestampDisplayName[ITEM_SONG_LULLABY],     "Nana de Zelda:      ");
    strcpy(timestampDisplayName[ITEM_SONG_EPONA],       "Canción de Epona:   ");
    strcpy(timestampDisplayName[ITEM_SONG_SARIA],       "Canción de Saria:   ");
    strcpy(timestampDisplayName[ITEM_SONG_SUN],         "Canción del Sol:    ");
    strcpy(timestampDisplayName[ITEM_SONG_TIME],        "Canción del tiempo: ");
    strcpy(timestampDisplayName[ITEM_SONG_STORMS],      "Canción tormenta:   ");
    strcpy(timestampDisplayName[ITEM_MEDALLION_FOREST], "Medallón Bosque:    ");
    strcpy(timestampDisplayName[ITEM_MEDALLION_FIRE],   "Medallón Fuego:     ");
    strcpy(timestampDisplayName[ITEM_MEDALLION_WATER],  "Medallón Agua:      ");
    strcpy(timestampDisplayName[ITEM_MEDALLION_SPIRIT], "Medallón Espíritu:  ");
    strcpy(timestampDisplayName[ITEM_MEDALLION_SHADOW], "Medallón Sombra:    ");
    strcpy(timestampDisplayName[ITEM_MEDALLION_LIGHT],  "Medallón Luz:       ");
    strcpy(timestampDisplayName[ITEM_KOKIRI_EMERALD],   "Esmeralda Kokiri:   ");
    strcpy(timestampDisplayName[ITEM_GORON_RUBY],       "Rubí de los Goron:  ");
    strcpy(timestampDisplayName[ITEM_ZORA_SAPPHIRE],    "Zafiro de los Zora: ");
    strcpy(timestampDisplayName[ITEM_KEY_BOSS],         "Gran llave Ganon:   ");
    strcpy(timestampDisplayName[ITEM_SINGLE_MAGIC],     "Magia:              ");
    strcpy(timestampDisplayName[ITEM_DOUBLE_DEFENSE],   "Doble defensa:      ");

    // Other events
    strcpy(timestampDisplayName[TIMESTAMP_DEFEAT_GOHMA],         "Gohma vencido:      ");
    strcpy(timestampDisplayName[TIMESTAMP_DEFEAT_KING_DODONGO],  "R. Dodongo vencido: ");
    strcpy(timestampDisplayName[TIMESTAMP_DEFEAT_BARINADE],      "Barinade vencido:   ");
    strcpy(timestampDisplayName[TIMESTAMP_DEFEAT_PHANTOM_GANON], "PG vencido:         ");
    strcpy(timestampDisplayName[TIMESTAMP_DEFEAT_VOLVAGIA],      "Volvagia vencido:   ");
    strcpy(timestampDisplayName[TIMESTAMP_DEFEAT_MORPHA],        "Morpha vencido:     ");
    strcpy(timestampDisplayName[TIMESTAMP_DEFEAT_BONGO_BONGO],   "Bongo vencido:      ");
    strcpy(timestampDisplayName[TIMESTAMP_DEFEAT_TWINROVA],      "Twinrova vencida:   ");
    strcpy(timestampDisplayName[TIMESTAMP_DEFEAT_GANONDORF],     "Ganondorf vencido:  ");
    strcpy(timestampDisplayName[TIMESTAMP_DEFEAT_GANON],         "Ganon vencido:      ");
    strcpy(timestampDisplayName[TIMESTAMP_FOUND_GREG],           "Greg hallado:       ");
}

void SetupDisplayColors() {
    for (int i = 0; i < TIMESTAMP_MAX; i++) {
        switch (i) {
            case ITEM_SONG_MINUET:
            case ITEM_KOKIRI_EMERALD:
            case ITEM_SONG_SARIA:
            case ITEM_MEDALLION_FOREST:
            case TIMESTAMP_FOUND_GREG:
                timestampDisplayColor[i] = COLOR_GREEN;
                break;
            case ITEM_SONG_BOLERO:
            case ITEM_GORON_RUBY:
            case ITEM_MEDALLION_FIRE:
                timestampDisplayColor[i] = COLOR_RED;
                break;
            case ITEM_SONG_SERENADE:
            case ITEM_ZORA_SAPPHIRE:
            case ITEM_MEDALLION_WATER:
                timestampDisplayColor[i] = COLOR_BLUE;
                break;
            case ITEM_SONG_LULLABY:
            case ITEM_SONG_NOCTURNE:
            case ITEM_MEDALLION_SHADOW:
                timestampDisplayColor[i] = COLOR_PURPLE;
                break;
            case ITEM_SONG_EPONA:
            case ITEM_SONG_REQUIEM:
            case ITEM_MEDALLION_SPIRIT:
                timestampDisplayColor[i] = COLOR_ORANGE;
                break;
            case ITEM_SONG_SUN:
            case ITEM_SONG_PRELUDE:
            case ITEM_MEDALLION_LIGHT:
            case ITEM_ARROW_LIGHT:
                timestampDisplayColor[i] = COLOR_YELLOW;
                break;
            case ITEM_SONG_STORMS:
                timestampDisplayColor[i] = COLOR_GREY;
                break;
            case ITEM_SONG_TIME:
                timestampDisplayColor[i] = COLOR_LIGHT_BLUE;
                break;
            default:
                timestampDisplayColor[i] = COLOR_WHITE;
                break;
        }
    }
}

void InitStatTracker() {
    SohImGui::AddWindow("Enhancements", "Gameplay Stats", DrawStatsTracker, CVarGetInteger("gGameplayStatsEnabled", 0) == 1);
    SetupDisplayNames();
    SetupDisplayColors();
}