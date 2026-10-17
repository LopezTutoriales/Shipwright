#include "debugconsole.h"
#include <ImGuiImpl.h>
#include <Utils.h>
#include "savestates.h"
#include <Console.h>

#include <vector>
#include <string>
#include "soh/OTRGlobals.h"
#include <soh/Enhancements/item-tables/ItemTableManager.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/cosmetics/CosmeticsEditor.h"
#include "soh/Enhancements/audio/AudioEditor.h"

#define Path _Path
#define PATH_HACK
#include <Utils/StringHelper.h>

#include <Window.h>
#include <ImGui/imgui_internal.h>
#undef PATH_HACK
#undef Path

extern "C" {
#include <z64.h>
#include "variables.h"
#include "functions.h"
#include "macros.h"
extern PlayState* gPlayState;
}

#include <libultraship/bridge.h>

#define CMD_REGISTER SohImGui::GetConsole()->AddCommand

static bool ActorSpawnHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if ((args.size() != 9) && (args.size() != 3) && (args.size() != 6)) {
        SohImGui::GetConsole()->SendErrorMessage("Faltan argumentos para actorspawn");
        return CMD_FAILED;
    }

    if (gPlayState == nullptr) {
        SohImGui::GetConsole()->SendErrorMessage("PlayState == nullptr");
        return CMD_FAILED;
    }

    Player* player = GET_PLAYER(gPlayState);
    PosRot spawnPoint;
    const s16 actorId = std::stoi(args[1]);
    const s16 params = std::stoi(args[2]);

    spawnPoint = player->actor.world;

    switch (args.size()) {
        case 9:
            if (args[6][0] != ',') {
                spawnPoint.rot.x = std::stoi(args[6]);
            }
            if (args[7][0] != ',') {
                spawnPoint.rot.y = std::stoi(args[7]);
            }
            if (args[8][0] != ',') {
                spawnPoint.rot.z = std::stoi(args[8]);
            }
        case 6:
            if (args[3][0] != ',') {
                spawnPoint.pos.x = std::stoi(args[3]);
            }
            if (args[4][0] != ',') {
                spawnPoint.pos.y = std::stoi(args[4]);
            }
            if (args[5][0] != ',') {
                spawnPoint.pos.z = std::stoi(args[5]);
            }
    }

    if (Actor_Spawn(&gPlayState->actorCtx, gPlayState, actorId, spawnPoint.pos.x, spawnPoint.pos.y, spawnPoint.pos.z,
                    spawnPoint.rot.x, spawnPoint.rot.y, spawnPoint.rot.z, params, 0) == NULL) {
        SohImGui::GetConsole()->SendErrorMessage("No se pudo generar el actor. Actor_Spawn devolvió NULL");
        return CMD_FAILED;
    }
    return CMD_SUCCESS;
}

static bool KillPlayerHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>&) {
    GameInteractionEffectBase* effect = new GameInteractionEffect::SetPlayerHealth();
    effect->parameters[0] = 0;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Has tenido un final terrible, ¿verdad?");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo matar al jugador.");
        return CMD_FAILED;
    }
}

static bool SetPlayerHealthHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    int health;

    try {
        health = std::stoi(args[1]);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de salud debe ser un entero.");
        return CMD_FAILED;
    }

    if (health < 0) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de salud debe ser un entero positivo");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::SetPlayerHealth();
    effect->parameters[0] = health;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Salud del jugador actualizada a %d", health);
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo establecer la salud del jugador.");
        return CMD_FAILED;
    }
}

static bool LoadSceneHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>&) {
    gSaveContext.respawnFlag = 0;
    gSaveContext.seqId = 0xFF;
    gSaveContext.gameMode = 0;

    return CMD_SUCCESS;
}

static bool RupeeHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return CMD_FAILED;
    }

    int rupeeAmount;
    try {
        rupeeAmount = std::stoi(args[1]);
    }
    catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] La cantidad de rupias debe ser un entero.");
        return CMD_FAILED;
    }

    if (rupeeAmount < 0) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] La cantidad de rupias debe ser positiva");
        return CMD_FAILED;
    }

   gSaveContext.rupees = rupeeAmount;

    SohImGui::GetConsole()->SendInfoMessage("Cantidad de rupias establecida en %u", rupeeAmount);
    return CMD_SUCCESS;
}

static bool SetPosHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string> args) {
    if (gPlayState == nullptr) {
        SohImGui::GetConsole()->SendErrorMessage("PlayState == nullptr");
        return CMD_FAILED;
    }

    Player* player = GET_PLAYER(gPlayState);

    if (args.size() == 1) {
        SohImGui::GetConsole()->SendInfoMessage("La posición del jugador es [ %.2f, %.2f, %.2f ]", player->actor.world.pos.x,
                                            player->actor.world.pos.y,
             player->actor.world.pos.z);
        return CMD_SUCCESS;
    }
    if (args.size() < 4)
        return CMD_FAILED;

    player->actor.world.pos.x = std::stof(args[1]);
    player->actor.world.pos.y = std::stof(args[2]);
    player->actor.world.pos.z = std::stof(args[3]);

    SohImGui::GetConsole()->SendInfoMessage("Posición del jugador establecida en [ %.2f, %.2f, %.2f ]", player->actor.world.pos.x,
                                        player->actor.world.pos.y,
         player->actor.world.pos.z);
    return CMD_SUCCESS;
}

static bool ResetHandler(std::shared_ptr<Ship::Console> Console, std::vector<std::string> args) {
    if (gPlayState == nullptr) {
        SohImGui::GetConsole()->SendErrorMessage("PlayState == nullptr");
        return CMD_FAILED;
    }

    SET_NEXT_GAMESTATE(&gPlayState->state, TitleSetup_Init, GameState);
    gPlayState->state.running = false;
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnExitGame>(gSaveContext.fileNum);
    return CMD_SUCCESS;
}

const static std::map<std::string, uint16_t> ammoItems{ 
    { "sticks", ITEM_STICK }, { "nuts", ITEM_NUT },
    { "bombs", ITEM_BOMB },   { "seeds", ITEM_SLINGSHOT },
    { "arrows", ITEM_BOW },   { "bombchus", ITEM_BOMBCHU },
    { "beans", ITEM_BEAN }
};

static bool AddAmmoHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 3) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    int amount;

    try {
        amount = std::stoi(args[2]);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("La cantidad de munición debe ser un entero");
        return CMD_FAILED;
    }

    if (amount < 0) {
        SohImGui::GetConsole()->SendErrorMessage("La cantidad de munición debe ser positiva");
        return CMD_FAILED;
    }

    const auto& it = ammoItems.find(args[1]);
    if (it == ammoItems.end()) {
        SohImGui::GetConsole()->SendErrorMessage("Tipo de munición inválido. Opciones: 'sticks', 'nuts', 'bombs', 'seeds', 'arrows', 'bombchus' y 'beans'");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::AddOrTakeAmmo();
    effect->parameters[0] = amount;
    effect->parameters[1] = it->second;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Munición añadida.");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo añadir munición.");
        return CMD_FAILED;
    }
}

static bool TakeAmmoHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 3) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    int amount;

    try {
        amount = std::stoi(args[2]);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("La cantidad de munición debe ser un entero");
        return CMD_FAILED;
    }

    if (amount < 0) {
        SohImGui::GetConsole()->SendErrorMessage("La cantidad de munición debe ser positiva");
        return CMD_FAILED;
    }

    const auto& it = ammoItems.find(args[1]);
    if (it == ammoItems.end()) {
        SohImGui::GetConsole()->SendErrorMessage(
            "Tipo de munición inválido. Opciones: 'sticks', 'nuts', 'bombs', 'seeds', 'arrows', 'bombchus' y 'beans'");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::AddOrTakeAmmo();
    effect->parameters[0] = -amount;
    effect->parameters[1] = it->second;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Munición retirada.");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo retirar munición.");
        return CMD_FAILED;
    }
}

const static std::map<std::string, uint16_t> bottleItems{
    { "green_potion", ITEM_POTION_GREEN },
    { "red_potion", ITEM_POTION_RED },
    { "blue_potion", ITEM_POTION_BLUE },
    { "milk", ITEM_MILK },    { "half_milk", ITEM_MILK_HALF },           { "fairy", ITEM_FAIRY },
    { "bugs", ITEM_BUG }, { "fish", ITEM_FISH },         { "poe", ITEM_POE },
    { "big_poe", ITEM_BIG_POE },  { "blue_fire", ITEM_BLUE_FIRE }, { "rutos_letter", ITEM_LETTER_RUTO },
};

static bool BottleHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 3) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }

    unsigned int slot;
    try {
        slot = std::stoi(args[2]);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] La ranura de botella debe ser un entero.");
        return CMD_FAILED;
    }

    if ((slot < 1) || (slot > 4)) {
        SohImGui::GetConsole()->SendErrorMessage("Ranura inválida");
        return CMD_FAILED;
    }

    const auto& it = bottleItems.find(args[1]);

    if (it ==  bottleItems.end()) {
        SohImGui::GetConsole()->SendErrorMessage("Objeto inválido");
        return CMD_FAILED;
    }

    // I dont think you can do OOB with just this
    gSaveContext.inventory.items[0x11 + slot] = it->second;

    return CMD_SUCCESS;
}

static bool BHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }

    gSaveContext.equips.buttonItems[0] = std::stoi(args[1]);
    return CMD_SUCCESS;
}

static bool ItemHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 3) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }

    gSaveContext.inventory.items[std::stoi(args[1])] = std::stoi(args[2]);

    return CMD_SUCCESS;
}

static bool GiveItemHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string> args) {
    if (args.size() != 3) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    GetItemEntry getItemEntry = GET_ITEM_NONE;

    if (args[1].compare("vanilla") == 0) {
        getItemEntry = ItemTableManager::Instance->RetrieveItemEntry(MOD_NONE, std::stoi(args[2]));
    } else if (args[1].compare("randomizer") == 0) {
        getItemEntry = ItemTableManager::Instance->RetrieveItemEntry(MOD_RANDOMIZER, std::stoi(args[2]));
    } else {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumento inválido: debe ser 'vanilla' o 'randomizer'");
        return CMD_FAILED;
    }

    GiveItemEntryWithoutActor(gPlayState, getItemEntry);

    return CMD_SUCCESS;
}

static bool EntranceHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }

    unsigned int entrance;

    try {
        entrance = std::stoi(args[1], nullptr, 16);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de entrada debe ser un número hexadecimal.");
        return CMD_FAILED;
    }

    gPlayState->nextEntranceIndex = entrance;
    gPlayState->sceneLoadFlag = 0x14;
    gPlayState->fadeTransition = 11;
    gSaveContext.nextTransitionType = 11;
}

static bool VoidHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (gPlayState != nullptr) {
            gSaveContext.respawn[RESPAWN_MODE_DOWN].tempSwchFlags = gPlayState->actorCtx.flags.tempSwch;
            gSaveContext.respawn[RESPAWN_MODE_DOWN].tempCollectFlags = gPlayState->actorCtx.flags.tempCollect;
            gSaveContext.respawnFlag = 1;
            gPlayState->sceneLoadFlag = 0x14;
            gPlayState->nextEntranceIndex = gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex;
            gPlayState->fadeTransition = 2;
            gSaveContext.nextTransitionType = 2;
    } else {
        SohImGui::GetConsole()->SendErrorMessage("gPlayState == nullptr");
        return CMD_FAILED;
    }
    return CMD_SUCCESS;
}

static bool ReloadHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (gPlayState != nullptr) {
        gPlayState->nextEntranceIndex = gSaveContext.entranceIndex;
        gPlayState->sceneLoadFlag = 0x14;
        gPlayState->fadeTransition = 11;
        gSaveContext.nextTransitionType = 11;
    } else {
        SohImGui::GetConsole()->SendErrorMessage("gPlayState == nullptr");
        return CMD_FAILED;
    }
    return CMD_SUCCESS;
}

const static std::map<std::string, uint16_t> fw_options {
    { "clear", 0}, {"warp", 1}, {"nackup", 2}
};

static bool FWHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }

    const auto& it = fw_options.find(args[1]);
    if (it == fw_options.end()) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Opción inválida. Opciones: 'clear', 'warp', 'backup'");
        return CMD_FAILED;
    }
    
    if (gPlayState != nullptr) {
        FaroresWindData clear = {};
        switch(it->second) {
            case 0: //clear
                gSaveContext.fw = clear;
                SohImGui::GetConsole()->SendInfoMessage("[SOH] ¡Punto del Viento de Farore borrado! Recarga la escena para aplicar el cambio.");
                return CMD_SUCCESS;
                break;
            case 1: //warp
                if (gSaveContext.respawn[RESPAWN_MODE_TOP].data > 0) {
                    gPlayState->sceneLoadFlag = 0x14;
                    gPlayState->nextEntranceIndex = gSaveContext.respawn[RESPAWN_MODE_TOP].entranceIndex;
                    gPlayState->fadeTransition = 5;
                } else {
                    SohImGui::GetConsole()->SendErrorMessage("¡Viento de Farore no colocado!");
                    return CMD_FAILED;
                }
                return CMD_SUCCESS;
                break;
            case 2: //backup
                if (CVarGetInteger("gBetterFW", 0)) {
                    gSaveContext.fw = gSaveContext.backupFW;
                    gSaveContext.fw.set = 1;
                    SohImGui::GetConsole()->SendInfoMessage("[SOH] ¡Datos de respaldo del Viento de Farore copiados! Recarga la escena para aplicar el cambio.");
                    return CMD_SUCCESS;
                } else {
                    SohImGui::GetConsole()->SendErrorMessage("¡El Viento de Farore mejorado no está activado!");
                    return CMD_FAILED;
                }
                break;
        }
    }
    else {
        SohImGui::GetConsole()->SendErrorMessage("gPlayState == nullptr");
        return CMD_FAILED;
    }
    
    return CMD_SUCCESS;
}

static bool FileSelectHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (gPlayState != nullptr) {
        SET_NEXT_GAMESTATE(&gPlayState->state, FileChoose_Init, FileChooseContext);
        gPlayState->state.running = 0;
    } else {
        SohImGui::GetConsole()->SendErrorMessage("gPlayState == nullptr");
        return CMD_FAILED;
    }
    return CMD_SUCCESS;
}

static bool QuitHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    Ship::Window::GetInstance()->Close();
    return CMD_SUCCESS;
}

static bool SaveStateHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    unsigned int slot = OTRGlobals::Instance->gSaveStateMgr->GetCurrentSlot();
    const SaveStateReturn rtn = OTRGlobals::Instance->gSaveStateMgr->AddRequest({ slot, RequestType::SAVE });

    switch (rtn) {
        case SaveStateReturn::SUCCESS:
            SohImGui::GetConsole()->SendInfoMessage("[SOH] Estado guardado en la ranura %u", slot);
            return CMD_SUCCESS;
        case SaveStateReturn::FAIL_WRONG_GAMESTATE:
            SohImGui::GetConsole()->SendErrorMessage("[SOH] No se puede guardar un estado fuera de \"GamePlay\"");
            return CMD_FAILED;
    }
}

static bool LoadStateHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    unsigned int slot = OTRGlobals::Instance->gSaveStateMgr->GetCurrentSlot();
    const SaveStateReturn rtn = OTRGlobals::Instance->gSaveStateMgr->AddRequest({ slot, RequestType::LOAD });

    switch (rtn) {
        case SaveStateReturn::SUCCESS:
            SohImGui::GetConsole()->SendInfoMessage("[SOH] Estado cargado de la ranura (%u)", slot);
            return CMD_SUCCESS;
        case SaveStateReturn::FAIL_INVALID_SLOT:
            SohImGui::GetConsole()->SendErrorMessage("[SOH] Número de ranura de estado inválido (%u)", slot);
            return CMD_FAILED;
        case SaveStateReturn::FAIL_STATE_EMPTY:
            SohImGui::GetConsole()->SendErrorMessage("[SOH] La ranura de estado (%u) está vacía", slot);
            return CMD_FAILED;
        case SaveStateReturn::FAIL_WRONG_GAMESTATE:
            SohImGui::GetConsole()->SendErrorMessage("[SOH] No se puede cargar un estado fuera de \"GamePlay\"");
            return CMD_FAILED;
    }

}

static bool StateSlotSelectHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    uint8_t slot;

    try {
        slot = std::stoi(args[1], nullptr, 10);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de la ranura debe ser un número.");
        return CMD_FAILED;
    }

    if (slot < 0) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Ranura inválida. Debe estar entre 0 y 2");
        return CMD_FAILED;
    }

    OTRGlobals::Instance->gSaveStateMgr->SetCurrentSlot(slot);
    SohImGui::GetConsole()->SendInfoMessage("[SOH] Ranura %u seleccionada",
                                        OTRGlobals::Instance->gSaveStateMgr->GetCurrentSlot());
    return CMD_SUCCESS;
}

static bool InvisibleHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de invisible debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::InvisibleLink();
    GameInteractionEffectQueryResult result = 
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Link invisible %s", state ? "activado" : "desactivado");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo %s Link invisible.",
                                                state ? "activar" : "desactivar");
        return CMD_FAILED;
    }
}

static bool GiantLinkHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de gigante debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyLinkSize();
    effect->parameters[0] = GI_LINK_SIZE_GIANT;
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Link gigante %s", state ? "activado" : "desactivado");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo %s Link gigante.",
                                                state ? "activar" : "desactivar");
        return CMD_FAILED;
    }
}

static bool MinishLinkHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de diminuto debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyLinkSize();
    effect->parameters[0] = GI_LINK_SIZE_MINISH;
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Link diminuto %s", state ? "activado" : "desactivado");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo %s Link diminuto.",
                                                state ? "activar" : "desactivar");
        return CMD_FAILED;
    }
}

static bool AddHeartContainerHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    int hearts;

    try {
        hearts = std::stoi(args[1]);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El número de corazones debe ser un entero.");
        return CMD_FAILED;
    }

    if (hearts < 0) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El número de corazones debe ser un entero positivo");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyHeartContainers();
    effect->parameters[0] = hearts;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Añadidos %d contenedores de corazón", hearts);
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudieron añadir contenedores de corazón.");
        return CMD_FAILED;
    }
}

static bool RemoveHeartContainerHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    int hearts;

    try {
        hearts = std::stoi(args[1]);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El número de corazones debe ser un entero.");
        return CMD_FAILED;
    }

    if (hearts < 0) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El número de corazones debe ser un entero positivo");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyHeartContainers();
    effect->parameters[0] = -hearts;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Retirados %d contenedores de corazón", hearts);
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudieron retirar contenedores de corazón.");
        return CMD_FAILED;
    }
}

static bool GravityHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyGravity();

    try {
        effect->parameters[0] = Ship::Math::clamp(std::stoi(args[1], nullptr, 10), GI_GRAVITY_LEVEL_LIGHT, GI_GRAVITY_LEVEL_HEAVY);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de gravedad debe ser un número.");
        return CMD_FAILED;
    }
    
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Gravedad actualizada.");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo actualizar la gravedad.");
        return CMD_FAILED;
    }
}

static bool NoUIHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de sin interfaz debe ser un número.");
        return CMD_FAILED;
    }
    
    GameInteractionEffectBase* effect = new GameInteractionEffect::NoUI();
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Sin interfaz %s", state ? "activado" : "desactivado");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo %s Sin interfaz.",
                                                state ? "activar" : "desactivar");
        return CMD_FAILED;
    }
}

static bool FreezeHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    GameInteractionEffectBase* effect = new GameInteractionEffect::FreezePlayer();
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Jugador congelado");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo congelar al jugador.");
        return CMD_FAILED;
    }
}

static bool DefenseModifierHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyDefenseModifier();

    try {
        effect->parameters[0] = std::stoi(args[1], nullptr, 10);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El modificador de defensa debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Modificador de defensa establecido en %d", effect->parameters[0]);
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo establecer el modificador de defensa.");
        return CMD_FAILED;
    }
}

static bool DamageHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyHealth();

    try {
        int value = std::stoi(args[1], nullptr, 10);
        if (value < 0) {
            SohImGui::GetConsole()->SendErrorMessage("[SOH] Valor inválido. Debe ser mayor que 0");
            return CMD_FAILED;
        }

        effect->parameters[0] = -value;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de daño debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Jugador dañado");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo dañar al jugador.");
        return CMD_FAILED;
    }
}

static bool HealHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyHealth();

    try {
        int value = std::stoi(args[1], nullptr, 10);
        if (value < 0) {
            SohImGui::GetConsole()->SendErrorMessage("[SOH] Valor inválido. Debe ser mayor que 0");
            return CMD_FAILED;
        }

        effect->parameters[0] = value;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de daño debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Jugador curado");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo curar al jugador.");
        return CMD_FAILED;
    }
}

static bool FillMagicHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    GameInteractionEffectBase* effect = new GameInteractionEffect::FillMagic();
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Magia rellenada");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo rellenar la magia.");
        return CMD_FAILED;
    }
}

static bool EmptyMagicHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    GameInteractionEffectBase* effect = new GameInteractionEffect::EmptyMagic();
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Magia vaciada");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo vaciar la magia.");
        return CMD_FAILED;
    }
}

static bool NoZHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
     if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
     uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de NoZ debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::DisableZTargeting();
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] NoZ " + std::string(state ? "activado" : "desactivado"));
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo " +
                                                std::string(state ? "activar" : "desactivar") + " NoZ.");
        return CMD_FAILED;
    }
}

static bool OneHitKOHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de KO de un golpe debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::OneHitKO();
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] KO de un golpe " + std::string(state ? "activado" : "desactivado"));
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo " +
                                                std::string(state ? "activar" : "desactivar") + " KO de un golpe.");
        return CMD_FAILED;
    }
}

static bool PacifistHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de pacifista debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::PacifistMode();
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Pacifista " + std::string(state ? "activado" : "desactivado"));
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo " +
                                                std::string(state ? "activar" : "desactivar") + " Pacifista.");
        return CMD_FAILED;
    }
}

static bool PaperLinkHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de Link de papel debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyLinkSize();
    effect->parameters[0] = GI_LINK_SIZE_PAPER;
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Link de papel " + std::string(state ? "activado" : "desactivado"));
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo " +
                                                std::string(state ? "activar" : "desactivar") + " Link de papel.");
        return CMD_FAILED;
    }
}

static bool RainstormHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de tormenta debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::WeatherRainstorm();
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Tormenta " + std::string(state ? "activado" : "desactivado"));
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo " +
                                                std::string(state ? "activar" : "desactivar") + " Tormenta.");
        return CMD_FAILED;
    }
}

static bool ReverseControlsHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    uint8_t state;

    try {
        state = std::stoi(args[1], nullptr, 10) == 0 ? 0 : 1;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de controles invertidos debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ReverseControls();
    GameInteractionEffectQueryResult result =
        state ? GameInteractor::ApplyEffect(effect) : GameInteractor::RemoveEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Controles invertidos " +
                                                std::string(state ? "activado" : "desactivado"));
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo " +
                                                std::string(state ? "activar" : "desactivar") + " Controles invertidos.");
        return CMD_FAILED;
    }
}

static bool UpdateRupeesHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyRupees();

    try {
        effect->parameters[0] = std::stoi(args[1], nullptr, 10);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de rupias debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Rupias actualizadas");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudieron actualizar las rupias.");
        return CMD_FAILED;
    }
}

static bool SpeedModifierHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    GameInteractionEffectBase* effect = new GameInteractionEffect::ModifyRunSpeedModifier();

    try {
        effect->parameters[0] = std::stoi(args[1], nullptr, 10);
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El modificador de velocidad debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Modificador de velocidad actualizado");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo actualizar el modificador de velocidad.");
        return CMD_FAILED;
    }
}

const static std::map<std::string, uint16_t> boots {
    { "kokiri", PLAYER_BOOTS_KOKIRI },
    { "iron", PLAYER_BOOTS_IRON },
    { "hover", PLAYER_BOOTS_HOVER },
};

static bool BootsHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }

    const auto& it = boots.find(args[1]);
    if (it == boots.end()) {
        SohImGui::GetConsole()->SendErrorMessage("Tipo de botas inválido. Opciones: 'kokiri', 'iron' y 'hover'");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::ForceEquipBoots();
    effect->parameters[0] = it->second;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Botas actualizadas.");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudieron actualizar las botas.");
        return CMD_FAILED;
    }
}

const static std::map<std::string, uint16_t> shields {
    { "deku", ITEM_SHIELD_DEKU },
    { "hylian", ITEM_SHIELD_HYLIAN },
    { "mirror", ITEM_SHIELD_MIRROR },
};

static bool GiveShieldHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }

    const auto& it = shields.find(args[1]);
    if (it == shields.end()) {
        SohImGui::GetConsole()->SendErrorMessage("Tipo de escudo inválido. Opciones: 'deku', 'hylian' y 'mirror'");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::GiveOrTakeShield();
    effect->parameters[0] = it->second;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Escudo entregado.");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo dar el escudo.");
        return CMD_FAILED;
    }
}

static bool TakeShieldHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }

    const auto& it = shields.find(args[1]);
    if (it == shields.end()) {
        SohImGui::GetConsole()->SendErrorMessage("Tipo de escudo inválido. Opciones: 'deku', 'hylian' y 'mirror'");
        return CMD_FAILED;
    }

    GameInteractionEffectBase* effect = new GameInteractionEffect::GiveOrTakeShield();
    effect->parameters[0] = it->second * -1;
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Escudo retirado.");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo retirar el escudo.");
        return CMD_FAILED;
    }
}

static bool KnockbackHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }
    GameInteractionEffectBase* effect = new GameInteractionEffect::KnockbackPlayer();

    try {
        int value = std::stoi(args[1], nullptr, 10);
        if (value < 0) {
            SohImGui::GetConsole()->SendErrorMessage("[SOH] Valor inválido. Debe ser mayor que 0");
            return CMD_FAILED;
        }

        effect->parameters[0] = value;
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de empuje debe ser un número.");
        return CMD_FAILED;
    }

    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);
    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Empuje aplicado");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo aplicar el empuje.");
        return CMD_FAILED;
    }
}

static bool ElectrocuteHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    GameInteractionEffectBase* effect = new GameInteractionEffect::ElectrocutePlayer();
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Jugador electrocutado");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo electrocutar al jugador.");
        return CMD_FAILED;
    }
}

static bool BurnHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    GameInteractionEffectBase* effect = new GameInteractionEffect::BurnPlayer();
    GameInteractionEffectQueryResult result = GameInteractor::ApplyEffect(effect);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Jugador quemado");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo quemar al jugador.");
        return CMD_FAILED;
    }
}

static bool CuccoStormHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    GameInteractionEffectQueryResult result = GameInteractor::RawAction::SpawnActor(ACTOR_EN_NIW, 0);

    if (result == GameInteractionEffectQueryResult::Possible) {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Tormenta de cucos generada");
        return CMD_SUCCESS;
    } else {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] Error: no se pudo generar la tormenta de cucos.");
        return CMD_FAILED;
    }
}

static bool GenerateRandoHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() == 1) {
        if (GenerateRandomizer()) {
            return CMD_SUCCESS;
        }
    }

    try {
        uint32_t value = std::stoi(args[1], NULL, 10);
        std::string seed = "";
        if (args.size() == 3) {
            int testing = std::stoi(args[1], nullptr, 10);
            seed = "seed_testing_count";
        }

        if (GenerateRandomizer(seed + std::to_string(value))){
            return CMD_SUCCESS;
        }
    } catch (std::invalid_argument const& ex) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] El valor de seed|count debe ser un número.");
        return CMD_FAILED;
    }


    SohImGui::GetConsole()->SendErrorMessage("[SOH] Ya hay una generación de randomizer en curso");
    return CMD_FAILED;
}

static bool CosmeticsHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }

    if (args[1].compare("reset") == 0) {
        CosmeticsEditor_ResetAll();
    } else if (args[1].compare("randomize") == 0) {
        CosmeticsEditor_RandomizeAll();
    } else {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumento inválido: debe ser 'reset' o 'randomize'");
        return CMD_FAILED;
    }

    return CMD_SUCCESS;
}

static bool SfxHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() != 2) {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumentos inesperados");
        return CMD_FAILED;
    }

    if (args[1].compare("reset") == 0) {
        AudioEditor_ResetAll();
    } else if (args[1].compare("randomize") == 0) {
        AudioEditor_RandomizeAll();
    } else {
        SohImGui::GetConsole()->SendErrorMessage("[SOH] Argumento inválido: debe ser 'reset' o 'randomize'");
        return CMD_FAILED;
    }

    return CMD_SUCCESS;
}

#define VARTYPE_INTEGER 0
#define VARTYPE_FLOAT   1
#define VARTYPE_STRING  2
#define VARTYPE_RGBA    3

static int CheckVarType(const std::string& input)
{
    int result = VARTYPE_STRING;

    if (input[0] == '#') {
        return VARTYPE_RGBA;
    }

    for (size_t i = 0; i < input.size(); i++)
    {
        if (!(std::isdigit(input[i]) || input[i] == '.'))
        {
            break;
        }
        else
        {
            if (input[i] == '.')
                result = VARTYPE_FLOAT;
            else if (std::isdigit(input[i]) && result != VARTYPE_FLOAT)
                result = VARTYPE_INTEGER;
        }
    }

    return result;
}

static bool SetCVarHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 3)
        return CMD_FAILED;

    int vType = CheckVarType(args[2]);

    if (vType == VARTYPE_STRING)
        CVarSetString(args[1].c_str(), args[2].c_str());
    else if (vType == VARTYPE_FLOAT)
        CVarSetFloat((char*)args[1].c_str(), std::stof(args[2]));
    else if (vType == VARTYPE_RGBA)
    {
        uint32_t val = std::stoul(&args[2].c_str()[1], nullptr, 16);
        Color_RGBA8 clr;
        clr.r = val >> 24;
        clr.g = val >> 16;
        clr.b = val >> 8;
        clr.a = val & 0xFF;
        CVarSetColor((char*)args[1].c_str(), clr);
    }
    else
        CVarSetInteger(args[1].c_str(), std::stoi(args[2]));

    CVarSave();

    //SohImGui::GetConsole()->SendInfoMessage("[SOH] Updated player position to [ %.2f, %.2f, %.2f ]", pos.x, pos.y, pos.z);
    return CMD_SUCCESS;
}

static bool GetCVarHandler(std::shared_ptr<Ship::Console> Console, const std::vector<std::string>& args) {
    if (args.size() < 2)
        return CMD_FAILED;

    auto cvar = CVarGet(args[1].c_str());

    if (cvar != nullptr)
    {
        if (cvar->Type == Ship::ConsoleVariableType::Integer)
            SohImGui::GetConsole()->SendInfoMessage("[SOH] La variable %s vale %i", args[1].c_str(), cvar->Integer);
        else if (cvar->Type == Ship::ConsoleVariableType::Float)
            SohImGui::GetConsole()->SendInfoMessage("[SOH] La variable %s vale %f", args[1].c_str(), cvar->Float);
        else if (cvar->Type == Ship::ConsoleVariableType::String)
            SohImGui::GetConsole()->SendInfoMessage("[SOH] La variable %s vale %s", args[1].c_str(), cvar->String.c_str());
        else if (cvar->Type == Ship::ConsoleVariableType::Color)
            SohImGui::GetConsole()->SendInfoMessage("[SOH] La variable %s vale %08X", args[1].c_str(), cvar->Color);
    }
    else
    {
        SohImGui::GetConsole()->SendInfoMessage("[SOH] No se encontró la variable %s", args[1].c_str());
    }


    return CMD_SUCCESS;
}

void DebugConsole_Init(void) {
    // Console
    CMD_REGISTER("file_select", { FileSelectHandler, "Vuelve a la selección de partida." });
    CMD_REGISTER("reset", { ResetHandler, "Reinicia el juego." });
    CMD_REGISTER("quit", { QuitHandler, "Cierra el juego." });

    // Save States
    CMD_REGISTER("save_state", { SaveStateHandler, "Guarda un estado." });
    CMD_REGISTER("load_state", { LoadStateHandler, "Carga un estado." });
    CMD_REGISTER("set_slot", { StateSlotSelectHandler, "Selecciona una ranura de estado", {
        { "Número de ranura", Ship::ArgumentType::NUMBER, }
    }});

    // Map & Location
    CMD_REGISTER("void", { VoidHandler, "Cae al vacío en el mapa actual." });
    CMD_REGISTER("reload", { ReloadHandler, "Recarga el mapa actual." });
    CMD_REGISTER("fw", { FWHandler,"Hace aparecer al jugador donde está el Viento de Farore." , {
        { "clear|warp|backup", Ship::ArgumentType::TEXT }
    }});
    CMD_REGISTER("entrance", { EntranceHandler, "Envía al jugador a la entrada indicada (hex)", {
        { "entrance", Ship::ArgumentType::NUMBER }
    }});

    // Gameplay
    CMD_REGISTER("kill", { KillPlayerHandler, "Suicidarse." });

    CMD_REGISTER("map",  { LoadSceneHandler, "¿Cargar Kakariko?" });

    CMD_REGISTER("rupee", { RupeeHandler, "Establece tu contador de rupias.", {
        {"amount", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("bItem", { BHandler, "Asigna un objeto al botón B.", {
        { "ID de objeto", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("spawn", { ActorSpawnHandler, "Genera un actor.", { { "actor_id", Ship::ArgumentType::NUMBER },
                              { "data", Ship::ArgumentType::NUMBER },
                              { "x", Ship::ArgumentType::PLAYER_POS, true },
                              { "y", Ship::ArgumentType::PLAYER_POS, true },
                              { "z", Ship::ArgumentType::PLAYER_POS, true },
                              { "rx", Ship::ArgumentType::PLAYER_ROT, true },
                              { "ry", Ship::ArgumentType::PLAYER_ROT, true },
                              { "rz", Ship::ArgumentType::PLAYER_ROT, true }
    }});

    CMD_REGISTER("pos", { SetPosHandler, "Establece la posición del jugador.", {
        { "x", Ship::ArgumentType::PLAYER_POS, true },
        { "y", Ship::ArgumentType::PLAYER_POS, true },
        { "z", Ship::ArgumentType::PLAYER_POS, true }
    }});

    CMD_REGISTER("set", { SetCVarHandler,  "Establece una variable de consola.", {
        { "varName", Ship::ArgumentType::TEXT },
        { "varValue", Ship::ArgumentType::TEXT }
    }});

    CMD_REGISTER("get", { GetCVarHandler, "Obtiene una variable de consola.", {
        { "varName", Ship::ArgumentType::TEXT }
    }});
    
    CMD_REGISTER("addammo", { AddAmmoHandler, "Añade munición de un objeto.", {
        { "sticks|nuts|bombs|seeds|arrows|bombchus|beans", Ship::ArgumentType::TEXT },
        { "count", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("takeammo", { TakeAmmoHandler, "Retira munición de un objeto.", {
        { "sticks|nuts|bombs|seeds|arrows|bombchus|beans", Ship::ArgumentType::TEXT },
        { "count", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("bottle", { BottleHandler, "Cambia el objeto de una ranura de botella.", {
        { "item", Ship::ArgumentType::TEXT },
        { "slot", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("give_item", { GiveItemHandler,  "Da un objeto al jugador como si lo diera un actor", {
        { "vanilla|randomizer", Ship::ArgumentType::TEXT },
        { "giveItemID", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("item", { ItemHandler,  "Coloca el ID de objeto del arg. 1 en la ranura del arg. 2. Sin comprobación de límites. ¡Cuidado!", {
        { "slot", Ship::ArgumentType::NUMBER },
        { "id de objeto", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("invisible", { InvisibleHandler, "Activa la capa élfica de Link, volviéndolo invisible.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("giant_link", { GiantLinkHandler, "Convierte a Link en un gigante.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("minish_link", { MinishLinkHandler, "Convierte a Link en un ser diminuto.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("add_heart_container", { AddHeartContainerHandler, "¡Da un corazón a Link! ¡El máximo es de 20 corazones!" });

    CMD_REGISTER("remove_heart_container", { RemoveHeartContainerHandler, "Quita un corazón a Link. El mínimo es de 3 corazones." });

    CMD_REGISTER("gravity", { GravityHandler, "Establece el nivel de gravedad.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("no_ui", { NoUIHandler, "Desactiva la interfaz.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("freeze", { FreezeHandler, "Congela a Link en el sitio" });

    CMD_REGISTER("defense_modifier", { DefenseModifierHandler, "Establece el modificador de defensa.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("damage", { DamageHandler, "Hace daño a Link.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("heal", { HealHandler, "Cura a Link.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("fill_magic", { FillMagicHandler, "Rellena la magia." });

    CMD_REGISTER("empty_magic", { EmptyMagicHandler, "Vacía la magia." });

    CMD_REGISTER("no_z", { NoZHandler, "Desactiva las pulsaciones del botón Z.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("ohko", { OneHitKOHandler, "Activa el KO de un golpe. Cualquier daño mata a Link y no puede recuperar salud en este modo.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("pacifist", { PacifistHandler, "Activa el modo pacifista. Impide que Link use su arma.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("paper_link", { PaperLinkHandler, "Link hecho de papel.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("rainstorm", { RainstormHandler, "Activa la tormenta." });

    CMD_REGISTER("reverse_controls", { ReverseControlsHandler, "Invierte los controles.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("update_rupees", { UpdateRupeesHandler, "Añade rupias.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("speed_modifier", { SpeedModifierHandler, "Establece el modificador de velocidad.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("boots", { BootsHandler, "Activa unas botas.", {
        { "kokiri|iron|hover", Ship::ArgumentType::TEXT },
    }});

    CMD_REGISTER("giveshield", { GiveShieldHandler, "Da un escudo y lo equipa si Link tiene la edad adecuada.", {
        { "deku|hylian|mirror", Ship::ArgumentType::TEXT },
    }});

    CMD_REGISTER("takeshield", { TakeShieldHandler, "Quita un escudo y lo desequipa si Link lo lleva.", {
        { "deku|hylian|mirror", Ship::ArgumentType::TEXT },
    }});

    CMD_REGISTER("knockback", { KnockbackHandler, "Empuja a Link hacia atrás.", {
        { "value", Ship::ArgumentType::NUMBER }
    }});

    CMD_REGISTER("electrocute", { ElectrocuteHandler, "Electrocuta a Link." });

    CMD_REGISTER("burn", { BurnHandler, "Quema a Link." });

    CMD_REGISTER("cucco_storm", { CuccoStormHandler, "Tormenta de cucos" });

    CMD_REGISTER("gen_rando", { GenerateRandoHandler, "Genera una semilla del randomizer", {
        { "seed|count", Ship::ArgumentType::NUMBER, true },
        { "testing", Ship::ArgumentType::NUMBER, true },
    }});

    CMD_REGISTER("cosmetics", { CosmeticsHandler, "Cambia los cosméticos.", {
        { "reset|randomize", Ship::ArgumentType::TEXT },
    }});

    CMD_REGISTER("sfx", { SfxHandler, "Cambia los efectos de sonido.", {
        { "reset|randomize", Ship::ArgumentType::TEXT },
    }});

    CVarLoad();
}
