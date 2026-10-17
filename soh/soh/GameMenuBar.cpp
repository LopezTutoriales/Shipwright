//
//  GameMenuBar.cpp
//  soh
//
//  Created by David Chavez on 24.08.22.
//

#include "GameMenuBar.hpp"

#include <spdlog/spdlog.h>
#include <ImGui/imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <ImGui/imgui_internal.h>
#include <ImGuiImpl.h>
#include <libultraship/bridge.h>
#include <Hooks.h>
#include <libultraship/libultra/types.h>
#include <libultraship/libultra/pi.h>
#include <libultraship/libultra/sptask.h>

#ifdef __SWITCH__
#include <port/switch/SwitchImpl.h>
#endif

#include "UIWidgets.hpp"
#include "include/global.h"
#include "include/z64audio.h"
#include "soh/SaveManager.h"
#include "OTRGlobals.h"
#include "soh/Enhancements/presets.h"

#ifdef ENABLE_CROWD_CONTROL
#include "Enhancements/crowd-control/CrowdControl.h"
#endif

#include "Enhancements/game-interactor/GameInteractor.h"

#define EXPERIMENTAL() \
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 50, 50, 255)); \
    UIWidgets::Spacer(3.0f); \
    ImGui::Text("Experimental"); \
    ImGui::PopStyleColor(); \
    UIWidgets::PaddedSeparator(false, true);

bool isBetaQuestEnabled = false;

extern "C" {
    void enableBetaQuest() { isBetaQuestEnabled = true; }
    void disableBetaQuest() { isBetaQuestEnabled = false; }
}

enum SeqPlayers {
    /* 0 */ SEQ_BGM_MAIN,
    /* 1 */ SEQ_FANFARE,
    /* 2 */ SEQ_SFX,
    /* 3 */ SEQ_BGM_SUB,
    /* 4 */ SEQ_MAX
};

namespace GameMenuBar {

    // MARK: - Properties

    const char* powers[9] = {
        "Original (1x)",
        "Doble (2x)",
        "Cuádruple (4x)",
        "Óctuple (8x)",
        "Hexadecúplo (16x)",
        "Duotrigintúplo (32x)",
        "Cuatrosexagintúplo (64x)",
        "Octoviginticentúplo (128x)",
        "Hexaquinquagintiducentúplo (256x)"
    };

    const char* bonkDamageValues[8] = {
        "Sin daño",
        "0,25 corazones",
        "0,5 corazones",
        "1 corazón",
        "2 corazones",
        "4 corazones",
        "8 corazones",
        "Muerte en 1 golpe"
    };

    // MARK: - Helpers

    std::string GetWindowButtonText(const char* text, bool menuOpen) {
        char buttonText[100] = "";
        if(menuOpen) { strcat(buttonText,"> "); }
        strcat(buttonText, text);
        if (!menuOpen) { strcat(buttonText, "  "); }
        return buttonText;
    }

    void BindAudioSlider(const char* name, const char* key, float defaultValue, SeqPlayers playerId) {
        float value = CVarGetFloat(key, defaultValue);

        ImGui::Text(name, static_cast<int>(100 * value));
        if (ImGui::SliderFloat((std::string("##") + key).c_str(), &value, 0.0f, 1.0f, "")) {
            const float volume = floorf(value * 100) / 100;
            CVarSetFloat(key, volume);
            SohImGui::RequestCvarSaveOnNextTick();
            Audio_SetGameVolume(playerId, volume);
        }
    }

    void UpdateAudio() {
        Audio_SetGameVolume(SEQ_BGM_MAIN, CVarGetFloat("gMainMusicVolume", 1));
        Audio_SetGameVolume(SEQ_BGM_SUB, CVarGetFloat("gSubMusicVolume", 1));
        Audio_SetGameVolume(SEQ_FANFARE, CVarGetFloat("gSFXMusicVolume", 1));
        Audio_SetGameVolume(SEQ_SFX, CVarGetFloat("gFanfareVolume", 1));
    }

    // MARK: - Delegates

    void SetupHooks() {
        Ship::RegisterHook<Ship::AudioInit>(UpdateAudio);
        Ship::RegisterHook<Ship::GfxInit>(UpdateAudio);
    }

    void Draw() {
        if (ImGui::BeginMenu("Ajustes"))
        {
            if (ImGui::BeginMenu("Audio")) {
                UIWidgets::EnhancementSliderFloat("Volumen general: %d %%", "##Master_Vol", "gGameMasterVolume", 0.0f, 1.0f, "", 1.0f, true);
                UIWidgets::Spacer(0);
                BindAudioSlider("Volumen música principal: %d %%", "gMainMusicVolume", 1.0f, SEQ_BGM_MAIN);
                UIWidgets::Spacer(0);
                BindAudioSlider("Volumen música secundaria: %d %%", "gSubMusicVolume", 1.0f, SEQ_BGM_SUB);
                UIWidgets::Spacer(0);
                BindAudioSlider("Volumen efectos de sonido: %d %%", "gSFXMusicVolume", 1.0f, SEQ_SFX);
                UIWidgets::Spacer(0);
                BindAudioSlider("Volumen fanfarrias: %d %%", "gFanfareVolume", 1.0f, SEQ_FANFARE);

                ImGui::Text("API de audio (requiere reinicio)");
                auto audioBackends = SohImGui::GetAvailableAudioBackends();
                auto currentAudioBackend = SohImGui::GetCurrentAudioBackend();

                if (audioBackends.size() <= 1) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }
                if (ImGui::BeginCombo("##AApi", currentAudioBackend.second)) {
                    for (uint8_t i = 0; i < audioBackends.size(); i++) {
                        if (ImGui::Selectable(audioBackends[i].second, audioBackends[i] == currentAudioBackend)) {
                            SohImGui::SetCurrentAudioBackend(i, audioBackends[i]);
                        }
                    }

                    ImGui::EndCombo();
                }
                if (audioBackends.size() <= 1) {
                    ImGui::PopItemFlag();
                    ImGui::PopStyleVar(1);
                }

                ImGui::EndMenu();
            }

            UIWidgets::Spacer(0);

            if (ImGui::BeginMenu("Mando")) {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2 (12.0f, 6.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.38f, 0.56f, 1.0f));
                if (ImGui::Button(GetWindowButtonText("Configuración del mando", CVarGetInteger("gControllerConfigurationEnabled", 0)).c_str(), ImVec2 (-1.0f, 0.0f)))
                {
                    bool currentValue = CVarGetInteger("gControllerConfigurationEnabled", 0);
                    CVarSetInteger("gControllerConfigurationEnabled", !currentValue);
                    SohImGui::RequestCvarSaveOnNextTick();
                    SohImGui::ToggleInputEditorWindow(CVarGetInteger("gControllerConfigurationEnabled", 0));
                }
                UIWidgets::PaddedSeparator();
                ImGui::PopStyleColor(1);
                ImGui::PopStyleVar(3);
            #ifndef __SWITCH__
                UIWidgets::EnhancementCheckbox("Navegar el menú con el mando", "gControlNav");
                UIWidgets::Tooltip("Permite navegar por la barra de menú de SoH (Ajustes, Mejoras...) con el mando.\nCUIDADO: se desactivarán los controles del juego mientras la barra de menú esté visible.\n\nCruceta para moverte entre opciones, A para seleccionar y X para dar el foco a la barra de menú");
            #endif
                UIWidgets::EnhancementCheckbox("Mostrar pulsaciones", "gInputEnabled");
                UIWidgets::Tooltip("Muestra los botones que estás pulsando en la esquina inferior derecha de la pantalla");
                UIWidgets::Spacer(0);
                ImGui::PushItemWidth(ImGui::GetWindowSize().x - 20.0f);
                UIWidgets::EnhancementSliderFloat("Escala de pulsaciones: %.1f", "##Input", "gInputScale", 1.0f, 3.0f, "", 1.0f, false);
                UIWidgets::Tooltip("Cambia el tamaño en pantalla de las pulsaciones mostradas con la opción \"Mostrar pulsaciones\"");
                ImGui::PopItemWidth();
                UIWidgets::Spacer(0);
                ImGui::PushItemWidth(ImGui::GetWindowSize().x - 20.0f);
                UIWidgets::EnhancementSliderInt("Retardo de entrada simulado: %d frames", "##SimulatedInputLag", "gSimulatedInputLag", 0, 6, "", 0, false);
                UIWidgets::Tooltip("Retrasa tus pulsaciones para ejecutarlas el número de frames indicado más tarde");
                ImGui::PopItemWidth();

                ImGui::EndMenu();
            }

            UIWidgets::Spacer(0);

            if (ImGui::BeginMenu("Gráficos")) {
            #ifndef __APPLE__
                UIWidgets::EnhancementSliderFloat("Resolución interna: %d %%", "##IMul", "gInternalResolution", 0.5f, 2.0f, "", 1.0f, true, true);
                UIWidgets::Tooltip("Multiplica la resolución de salida por el valor indicado; es una forma de antialiasing más exigente pero eficaz");
                SohImGui::SetResolutionMultiplier(CVarGetFloat("gInternalResolution", 1));
            #endif
            #ifndef __WIIU__
                UIWidgets::PaddedEnhancementSliderInt("MSAA: %d", "##IMSAA", "gMSAAValue", 1, 8, "", 1, false, true, false);
                UIWidgets::Tooltip("Activa el antialiasing multimuestra cuando es superior a 1x, hasta 8x (8 muestras por cada píxel)");
                SohImGui::SetMSAALevel(CVarGetInteger("gMSAAValue", 1));
            #endif

                if (SohImGui::WindowBackend() == SohImGui::Backend::DX11)
                {
                    const char* cvar = "gExtraLatencyThreshold";
                    int val = CVarGetInteger(cvar, 80);
                    val = fmax(fmin(val, 360), 0);
                    int fps = val;

                    UIWidgets::Spacer(0);

                    if (fps == 0)
                    {
                        ImGui::Text("Corrección de jitter: No");
                    }
                    else
                    {
                        ImGui::Text("Corrección de jitter: >= %d FPS", fps);
                    }

                    std::string MinusBTNELT = " - ##ExtraLatencyThreshold";
                    std::string PlusBTNELT = " + ##ExtraLatencyThreshold";
                    if (ImGui::Button(MinusBTNELT.c_str())) {
                        val--;
                        CVarSetInteger(cvar, val);
                        SohImGui::RequestCvarSaveOnNextTick();
                    }
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 7.0f);
                #ifdef __SWITCH__
                    ImGui::PushItemWidth(ImGui::GetWindowSize().x - 110.0f);
                #elif defined(__WIIU__)
                    ImGui::PushItemWidth(ImGui::GetWindowSize().x - 79.0f * 2);
                #else
                    ImGui::PushItemWidth(ImGui::GetWindowSize().x - 79.0f);
                #endif
                    if (ImGui::SliderInt("##ExtraLatencyThreshold", &val, 0, 360, "", ImGuiSliderFlags_AlwaysClamp))
                    {
                        CVarSetInteger(cvar, val);
                        SohImGui::RequestCvarSaveOnNextTick();
                    }
                    ImGui::PopItemWidth();
                    UIWidgets::Tooltip("Cuando los FPS de interpolación igualan o superan este umbral, se añade un frame de retardo de entrada (p. ej., 16,6 ms a 60 FPS) para evitar el jitter. Así la CPU puede trabajar en un frame mientras la GPU procesa el anterior.\nUsa esta opción si tu equipo es demasiado lento para hacer el trabajo de CPU + GPU a tiempo.");

                    ImGui::SameLine();
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 7.0f);
                    if (ImGui::Button(PlusBTNELT.c_str())) {
                        val++;
                        CVarSetInteger(cvar, val);
                        SohImGui::RequestCvarSaveOnNextTick();
                    }

                    UIWidgets::Spacer(0);
                }

                ImGui::Text("API de renderizado (requiere reinicio)");
                auto renderingBackends = SohImGui::GetAvailableRenderingBackends();
                auto currentRenderingBackend = SohImGui::GetCurrentRenderingBackend();

                if (renderingBackends.size() <= 1) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }
                if (ImGui::BeginCombo("##RApi", currentRenderingBackend.second)) {
                    for (uint8_t i = 0; i < renderingBackends.size(); i++) {
                        if (ImGui::Selectable(renderingBackends[i].second, renderingBackends[i] == currentRenderingBackend)) {
                            SohImGui::SetCurrentRenderingBackend(i, renderingBackends[i]);
                        }
                    }

                    ImGui::EndCombo();
                }
                if (renderingBackends.size() <= 1) {
                    ImGui::PopItemFlag();
                    ImGui::PopStyleVar(1);
                }

                if (SohImGui::SupportsWindowedFullscreen()) {
                    UIWidgets::PaddedEnhancementCheckbox("Pantalla completa en ventana", "gSdlWindowedFullscreen", true, false);
                }

                if (SohImGui::SupportsViewports()) {
                    UIWidgets::PaddedEnhancementCheckbox("Permitir multiventana", "gEnableMultiViewports", true, false);
                    UIWidgets::Tooltip("Permite arrastrar las ventanas fuera de la ventana principal del juego. Requiere reiniciar para que surta efecto.");
                }

                EXPERIMENTAL();

                ImGui::Text("Filtro de texturas (requiere reinicio)");
                UIWidgets::EnhancementCombobox("gTextureFilter", SohImGui::GetSupportedTextureFilters(), 3, 0);

                UIWidgets::Spacer(0);

                SohImGui::DrawSettings();

                ImGui::EndMenu();
            }

            UIWidgets::Spacer(0);

            if (ImGui::BeginMenu("Idiomas")) {
                UIWidgets::PaddedEnhancementCheckbox("Traducir pantalla de título", "gTitleScreenTranslation");
                if (UIWidgets::EnhancementRadioButton("Inglés", "gLanguages", LANGUAGE_ENG)) {
                    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnSetGameLanguage>();
                }
                if (UIWidgets::EnhancementRadioButton("Alemán", "gLanguages", LANGUAGE_GER)) {
                    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnSetGameLanguage>();
                }
                if (UIWidgets::EnhancementRadioButton("Francés", "gLanguages", LANGUAGE_FRA)) {
                    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnSetGameLanguage>();
                }
                ImGui::EndMenu();
            }
            
            UIWidgets::Spacer(0);
            
            if (ImGui::BeginMenu("Accesibilidad")) {
            #if defined(_WIN32) || defined(__APPLE__)
                UIWidgets::PaddedEnhancementCheckbox("Texto a voz", "gA11yTTS");
                UIWidgets::Tooltip("Activa la lectura en voz alta de los diálogos del juego");
            #endif
                UIWidgets::PaddedEnhancementCheckbox("Sin recentrar cámara en reposo", "gA11yDisableIdleCam");
                UIWidgets::Tooltip("Desactiva el recentrado automático de la cámara cuando Link está parado.");
                
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::SetCursorPosY(0.0f);

        if (ImGui::BeginMenu("Mejoras"))
        {
            DrawPresetSelector(PRESET_TYPE_ENHANCEMENTS);

            UIWidgets::Spacer(0);

            if (ImGui::BeginMenu("Jugabilidad"))
            {
                if (ImGui::BeginMenu("Ahorro de tiempo"))
                {
                    UIWidgets::PaddedEnhancementSliderInt("Velocidad del texto: %dx", "##TEXTSPEED", "gTextSpeed", 1, 5, "", 1, false, false, true);
                    UIWidgets::PaddedEnhancementSliderInt("Velocidad del Rey Zora: %dx", "##MWEEPSPEED", "gMweepSpeed", 1, 5, "", 1, false, false, true);
                    UIWidgets::EnhancementSliderInt("Forja de Biggoron: %d días", "##FORGETIME", "gForgeTime", 0, 3, "", 3);
                    UIWidgets::Tooltip("Permite cambiar los días que tarda Biggoron en forjar la Espada de Biggoron");
                    UIWidgets::PaddedEnhancementSliderInt("Velocidad de escalada +%d", "##CLIMBSPEED", "gClimbSpeed", 0, 12, "", 0, false, false, true);
                    UIWidgets::PaddedEnhancementSliderInt("Velocidad al empujar bloques +%d", "##BLOCKSPEED", "gFasterBlockPush", 0, 5, "", 0, false, false, true);
                    UIWidgets::PaddedEnhancementCheckbox("Levantar rocas pesadas rápido", "gFasterHeavyBlockLift", true, false);
                    UIWidgets::Tooltip("Acelera el levantamiento de rocas plateadas y obeliscos");
                    UIWidgets::PaddedEnhancementCheckbox("Link como nombre predeterminado", "gLinkDefaultName");
                    UIWidgets::Tooltip("Permite tener \"Link\" como nombre de archivo predefinido");
                    UIWidgets::PaddedEnhancementCheckbox("Sin Navi forzada", "gNoForcedNavi", true, false);
                    UIWidgets::Tooltip("Evita las conversaciones forzadas con Navi");
                    UIWidgets::PaddedEnhancementCheckbox("Sin parón al coger Skulltulas", "gSkulltulaFreeze", true, false);
                    UIWidgets::Tooltip("Evita que el juego inmovilice al jugador al recoger Skulltulas Doradas");
                    UIWidgets::PaddedEnhancementCheckbox("Cofres rápidos", "gFastChests", true, false);
                    UIWidgets::Tooltip("Abre todos los cofres de una patada");
                    UIWidgets::PaddedText("Cofre acorde a su contenido", true, false);
                    const char* chestSizeAndTextureMatchesContentsOptions[4] = { "Desactivado", "Ambos", "Solo textura", "Solo tamaño"};
                    UIWidgets::Tooltip(
                        "Se cambian el tamaño y la textura de los cofres para ayudar a identificar el objeto que contienen.\n"
                        " - Objetos importantes: cofres dorados grandes\n"
                        " - Objetos menores: cofres marrones grandes\n"
                        " - Objetos basura: cofres marrones pequeños\n"
                        " - Llaves pequeñas: cofre plateado pequeño\n"
                        " - Gran llave: tamaño y textura originales\n"
                        " - Símbolos de Skulltula: cofre de Skulltula pequeño\n"
                    );
                    if (UIWidgets::EnhancementCombobox("gChestSizeAndTextureMatchesContents", chestSizeAndTextureMatchesContentsOptions, 4, 0)) {
                        if (CVarGetInteger("gChestSizeAndTextureMatchesContents", 0) == 0) {
                            CVarSetInteger("gChestSizeDependsStoneOfAgony", 0);
                        }
                    }
                    if (CVarGetInteger("gChestSizeAndTextureMatchesContents", 0) > 0) {
                        UIWidgets::PaddedEnhancementCheckbox("Cofres de la Agonía", "gChestSizeDependsStoneOfAgony", true, false);
                        UIWidgets::Tooltip("Solo cambia el tamaño y la textura de los cofres si tienes la Piedra de la Agonía.");
                    }
                    UIWidgets::PaddedEnhancementCheckbox("Omitir mensajes de recogida", "gFastDrops", true, false);
                    UIWidgets::Tooltip("Omite los mensajes al recoger consumibles nuevos y al capturar cosas con botellas");
                    UIWidgets::PaddedEnhancementCheckbox("Preguntar si equipar lo nuevo", "gAskToEquip", true, false);
                    UIWidgets::Tooltip("Añade una pregunta para equipar las espadas, escudos y sayos recién obtenidos");
                    UIWidgets::PaddedEnhancementCheckbox("Búho mejorado", "gBetterOwl", true, false);
                    UIWidgets::Tooltip("La respuesta por defecto a Kaepora Gaebora es siempre que has entendido lo que dice");
                    UIWidgets::PaddedEnhancementCheckbox("Reproducción rápida de ocarina", "gFastOcarinaPlayback", true, false);
                    bool forceSkipScarecrow = gSaveContext.n64ddFlag &&
                        OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SKIP_SCARECROWS_SONG);
                    const char* forceSkipScarecrowText =
                        "Esta opción está activada a la fuerza porque hay una partida\ncargada con \"Omitir Son del espantapájaros\"";
                    UIWidgets::Tooltip("Omite la parte en la que se repite con la ocarina lo que acabas de tocar");
                    UIWidgets::PaddedEnhancementCheckbox("Omitir Son del espantapájaros", "gSkipScarecrow", true, false,
                                                         forceSkipScarecrow, forceSkipScarecrowText, UIWidgets::CheckboxGraphics::Checkmark);
                    UIWidgets::Tooltip("Pierre aparece al sacar la ocarina. Requiere haber aprendido el Son del espantapájaros.");
                    UIWidgets::PaddedEnhancementCheckbox("Recordar lugar de guardado", "gRememberSaveLocation", true, false);
                    UIWidgets::Tooltip("Al cargar una partida, coloca a Link en la última entrada por la que pasó.\n"
                            "No funciona si se guardó en una gruta.");
                    UIWidgets::PaddedEnhancementCheckbox("Omitir animación de flecha mágica", "gSkipArrowAnimation", true, false);
                    UIWidgets::PaddedEnhancementCheckbox("Omitir confirmación de guardado", "gSkipSaveConfirmation", true, false);
                    UIWidgets::Tooltip("Omite la pantalla de confirmación \"Partida guardada.\"");
                    UIWidgets::PaddedEnhancementCheckbox("Salir de la plaza de noche", "gMarketSneak", true, false);
                    UIWidgets::Tooltip("Permite salir de la plaza del mercado a la Llanura de Hyrule de noche hablando "
                    "con el guardia que hay junto a la puerta.");
                    UIWidgets::PaddedEnhancementCheckbox("Viento de Farore rápido", "gFastFarores", true, false);
                    UIWidgets::Tooltip("Reduce mucho el tiempo de lanzamiento del Viento de Farore.");
                    ImGui::EndMenu();
                }

                UIWidgets::Spacer(0);

                if (ImGui::BeginMenu("Objetos"))
                {
                    UIWidgets::PaddedEnhancementCheckbox("Guardar objeto al instante", "gInstantPutaway", true, false);
                    UIWidgets::Tooltip("Permite a Link guardar objetos sin tener que esperar");
                    UIWidgets::PaddedEnhancementCheckbox("Recuperar bumerán al instante", "gFastBoomerang", true, false);
                    UIWidgets::Tooltip("El bumerán vuelve a Link al instante si pulsas su botón de objeto mientras está en el aire");
                    UIWidgets::PaddedEnhancementCheckbox("Evitar pulsaciones perdidas (ocarina)", "gDpadNoDropOcarinaInput", true, false);
                    UIWidgets::Tooltip("Evita que se pierdan pulsaciones al tocar la ocarina rápidamente");
                    UIWidgets::PaddedText("Efecto de la Capucha de conejo", true, false);
                    const char* bunnyHoodOptions[3] = { "Desactivado", "Correr y saltar más", "Correr más rápido"};
                    UIWidgets::EnhancementCombobox("gMMBunnyHood", bunnyHoodOptions, 3, 0);
                    UIWidgets::Tooltip(
                        "Llevar la Capucha de conejo aumenta la velocidad, como en Majora's Mask. La opción de salto más largo no se tiene en cuenta en la lógica del randomizer.\n\n"
                        "También desactiva las reacciones de los personajes al llevar la Capucha de conejo."
                    );
                    UIWidgets::PaddedEnhancementCheckbox("Elegir máscara en el inventario", "gMaskSelect", true, false);
                    UIWidgets::Tooltip("Tras completar la misión secundaria de las máscaras, pulsa A y una dirección sobre la casilla de la máscara para cambiar de máscara");
                    UIWidgets::PaddedEnhancementCheckbox("Nueces hacen explotar bombas", "gNutsExplodeBombs", true, false);
                    UIWidgets::Tooltip("Hace que las nueces deku hagan explotar las bombas, igual que con los bombuchus. No afecta a las flores bomba.");
                    UIWidgets::PaddedEnhancementCheckbox("Equipar varias flechas a la vez", "gSeparateArrows", true, false);
                    UIWidgets::Tooltip("Permite equipar el arco y las flechas mágicas a la vez en casillas distintas");
                    UIWidgets::PaddedEnhancementCheckbox("Arco niño / Tirachinas adulto", "gBowSlingShotAmmoFix", true, false);
                    UIWidgets::Tooltip("Permite al Link niño usar el arco con flechas.\nPermite al Link adulto usar el tirachinas con semillas.\n\nPara equiparlos hacen falta glitches o el truco 'Equipamiento atemporal'.");
                    UIWidgets::PaddedEnhancementCheckbox("Viento de Farore mejorado", "gBetterFW", true, false);
                    UIWidgets::Tooltip("Ayuda a que el Viento de Farore persista entre edades, da puntos distintos para niño y adulto y se puede usar en más lugares.");
                    UIWidgets::PaddedEnhancementCheckbox("Radio de explosión fijo", "gStaticExplosionRadius", true, false);
                    UIWidgets::Tooltip("Las explosiones tienen un tamaño fijo, como en Majora's Mask y OoT3D. Facilita mucho el hovering con bombuchus.");
                    UIWidgets::PaddedEnhancementCheckbox("Bombuchus sin primera persona", "gDisableFirstPersonChus", true, false);
                    UIWidgets::Tooltip("Evita que los bombuchus fuercen la cámara en primera persona al lanzarlos.");
                    ImGui::EndMenu();
                }

                UIWidgets::Spacer(0);

                if (ImGui::BeginMenu("Dificultad"))
                {
                    ImGui::Text("Multiplicador de daño");
                    UIWidgets::EnhancementCombobox("gDamageMul", powers, 9, 0);
                    UIWidgets::Tooltip(
                        "Modifica todo el daño que no dependa de otros deslizadores\n\
                        2x: Sobrevives a todos los ataques comunes desde el inicio del juego\n\
                        4x: Un ataque fuerte te mata de un golpe desde el inicio del juego\n\
                        8x: Solo sobrevives a daño trivial desde el inicio del juego\n\
                        16x: Sobrevives a todos los ataques comunes con la salud al máximo y sin doble defensa\n\
                        32x: Sobrevives a todos los ataques comunes con la salud al máximo y con doble defensa\n\
                        64x: Sobrevives a daño trivial con la salud al máximo y sin doble defensa\n\
                        128x: Sobrevives a daño trivial con la salud al máximo y con doble defensa\n\
                        256x: No sobrevives a ningún daño"
                    );
                    UIWidgets::PaddedText("Multiplicador de daño por caída", true, false);
                    UIWidgets::EnhancementCombobox("gFallDamageMul", powers, 8, 0);
                    UIWidgets::Tooltip(
                        "Modifica todo el daño por caída\n\
                        2x: Sobrevives a todas las caídas desde el inicio del juego\n\
                        4x: Solo sobrevives a caídas cortas desde el inicio del juego\n\
                        8x: No sobrevives a ninguna caída desde el inicio del juego\n\
                        16x: Sobrevives a todas las caídas con la salud al máximo y sin doble defensa\n\
                        32x: Sobrevives a todas las caídas con la salud al máximo y con doble defensa\n\
                        64x: Sobrevives a caídas cortas con doble defensa\n\
                        128x: No sobrevives a ninguna caída"
                    );
                    UIWidgets::PaddedText("Multiplicador de daño por vacío", true, false);
                    UIWidgets::EnhancementCombobox("gVoidDamageMul", powers, 7, 0);
                    UIWidgets::Tooltip(
                        "Modifica el daño recibido tras caer al vacío\n\
                        2x: Sobrevives al daño del vacío desde el inicio del juego\n\
                        4x: No sobrevives al daño del vacío desde el inicio del juego\n\
                        8x: Sobrevives dos veces al daño del vacío con la salud al máximo y sin doble defensa\n\
                        16x: Sobrevives al daño del vacío con la salud al máximo y sin doble defensa\n\
                        32x: Sobrevives al daño del vacío con la salud al máximo y con doble defensa\n\
                        64x: No sobrevives al daño del vacío"
                    );
                    UIWidgets::PaddedText("Multiplicador de daño por choque", true, false);
                    UIWidgets::EnhancementCombobox("gBonkDamageMul", bonkDamageValues, 8, 0);
                    UIWidgets::Tooltip("Modifica el daño recibido tras chocar contra algo.");
                    UIWidgets::PaddedEnhancementCheckbox("Reaparecer con salud completa", "gFullHealthSpawn", true, false);
                    UIWidgets::Tooltip("Reaparece con todos los corazones en lugar de con 3");
                    UIWidgets::PaddedEnhancementCheckbox("Sin botín aleatorio", "gNoRandomDrops", true, false);
                    UIWidgets::Tooltip("Desactiva el botín aleatorio, excepto el de la vasija goron, el de Dampé y el de los jefes");
                    bool forceEnableBombchuDrops = gSaveContext.n64ddFlag &&
                        OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_ENABLE_BOMBCHU_DROPS) == 1;
                    const char* forceEnableBombchuDropsText =
                        "Esta opción está activada a la fuerza porque hay una partida\ncargada con \"Activar botín de bombuchus\".";
                    UIWidgets::PaddedEnhancementCheckbox("Activar botín de bombuchus", "gBombchuDrops", true, false,
                                                         forceEnableBombchuDrops, forceEnableBombchuDropsText, UIWidgets::CheckboxGraphics::Checkmark);
                    UIWidgets::Tooltip("A veces caerán bombuchus en lugar de bombas");
                    UIWidgets::PaddedEnhancementCheckbox("Sin corazones sueltos", "gNoHeartDrops", true, false);
                    UIWidgets::Tooltip("Desactiva los corazones que sueltan los enemigos, pero no los colocados en el mapa, como el de un mercader deku que huye\nSimula el Modo Héroe de otros juegos de la saga");
                    UIWidgets::PaddedEnhancementCheckbox("Ganar siempre vasija goron", "gGoronPot", true, false);
                    UIWidgets::Tooltip("Consigues siempre la pieza de corazón o la rupia morada de la vasija goron giratoria");
                    UIWidgets::PaddedEnhancementCheckbox("Ganar siempre excavación de Dampé", "gDampeWin", true, false, SaveManager::Instance->IsRandoFile(),
                                                         "Esta opción siempre está activada en partidas randomizer", UIWidgets::CheckboxGraphics::Checkmark);
                    UIWidgets::Tooltip("Ganas siempre la pieza de corazón o la rupia morada en la primera excavación del juego de las tumbas de Dampé, igual que en el randomizer\nEn una partida randomizer está activado siempre");
                    UIWidgets::Spacer(0);

                    if (ImGui::BeginMenu("Valores de pociones"))
                    {
                        UIWidgets::EnhancementCheckbox("Cambiar efecto de poción roja", "gRedPotionEffect");
                        UIWidgets::Tooltip("Activa los siguientes cambios en la salud que restauran las pociones rojas");
                        bool disabledRedPotion = !CVarGetInteger("gRedPotionEffect", 0);
                        const char* disabledTooltipRedPotion = "Opción desactivada porque \"Cambiar efecto de poción roja\" está desactivado";
                        UIWidgets::EnhancementSliderInt("Salud de poción roja: %d", "##REDPOTIONHEALTH", "gRedPotionHealth", 1, 100, "", 0, true, disabledRedPotion, disabledTooltipRedPotion);
                        UIWidgets::Tooltip("Cambia la salud que restauran las pociones rojas");
                        UIWidgets::EnhancementCheckbox("Poción roja: restauración porcentual", "gRedPercentRestore", disabledRedPotion, disabledTooltipRedPotion);
                        UIWidgets::Tooltip("Cambia las pociones rojas de restaurar una cantidad fija de salud a restaurar un porcentaje de la salud máxima actual del jugador");

                        ImGui::Separator();

                        UIWidgets::EnhancementCheckbox("Cambiar efecto de poción verde", "gGreenPotionEffect");
                        UIWidgets::Tooltip("Activa los siguientes cambios en la magia que restauran las pociones verdes");
                        bool disabledGreenPotion = !CVarGetInteger("gGreenPotionEffect", 0);
                        const char* disabledTooltipGreenPotion = "Opción desactivada porque \"Cambiar efecto de poción verde\" está desactivado";
                        UIWidgets::EnhancementSliderInt("Magia de poción verde: %d", "##GREENPOTIONMANA", "gGreenPotionMana", 1, 100, "", 0, true, disabledGreenPotion, disabledTooltipGreenPotion);
                        UIWidgets::Tooltip("Cambia la magia que restauran las pociones verdes. La magia máxima base es 48 y la máxima mejorada, 96");
                        UIWidgets::EnhancementCheckbox("Poción verde: restauración porcentual", "gGreenPercentRestore", disabledGreenPotion, disabledTooltipGreenPotion);
                        UIWidgets::Tooltip("Cambia las pociones verdes de restaurar una cantidad fija de magia a restaurar un porcentaje de la magia máxima actual del jugador");

                        ImGui::Separator();

                        UIWidgets::EnhancementCheckbox("Cambiar efectos de poción azul", "gBluePotionEffects");
                        UIWidgets::Tooltip("Activa los siguientes cambios en la salud y la magia que restauran las pociones azules");
                        bool disabledBluePotion = !CVarGetInteger("gBluePotionEffects", 0);
                        const char* disabledTooltipBluePotion = "Opción desactivada porque \"Cambiar efectos de poción azul\" está desactivado";
                        UIWidgets::EnhancementSliderInt("Salud de poción azul: %d", "##BLUEPOTIONHEALTH", "gBluePotionHealth", 1, 100, "", 0, true, disabledBluePotion, disabledTooltipBluePotion);
                        UIWidgets::Tooltip("Cambia la salud que restauran las pociones azules");
                        UIWidgets::EnhancementCheckbox("Poción azul (salud): restauración porcentual", "gBlueHealthPercentRestore", disabledBluePotion, disabledTooltipBluePotion);
                        UIWidgets::Tooltip("Cambia las pociones azules de restaurar una cantidad fija de salud a restaurar un porcentaje de la salud máxima actual del jugador");

                        ImGui::Separator();

                        UIWidgets::EnhancementSliderInt("Magia de poción azul: %d", "##BLUEPOTIONMANA", "gBluePotionMana", 1, 100, "", 0, true, disabledBluePotion, disabledTooltipBluePotion);
                        UIWidgets::Tooltip("Cambia la magia que restauran las pociones azules. La magia máxima base es 48 y la máxima mejorada, 96");
                        UIWidgets::EnhancementCheckbox("Poción azul (magia): restauración porcentual", "gBlueManaPercentRestore", disabledBluePotion, disabledTooltipBluePotion);
                        UIWidgets::Tooltip("Cambia las pociones azules de restaurar una cantidad fija de magia a restaurar un porcentaje de la magia máxima actual del jugador");

                        ImGui::Separator();

                        UIWidgets::EnhancementCheckbox("Cambiar efecto de la leche", "gMilkEffect");
                        UIWidgets::Tooltip("Activa los siguientes cambios en la salud que restaura la leche");
                        bool disabledMilk = !CVarGetInteger("gMilkEffect", 0);
                        const char* disabledTooltipMilk = "Opción desactivada porque \"Cambiar efecto de la leche\" está desactivado";
                        UIWidgets::EnhancementSliderInt("Salud de la leche: %d", "##MILKHEALTH", "gMilkHealth", 1, 100, "", 0, true, disabledMilk, disabledTooltipMilk);
                        UIWidgets::Tooltip("Cambia la salud que restaura la leche");
                        UIWidgets::EnhancementCheckbox("Leche: restauración porcentual", "gMilkPercentRestore", disabledMilk, disabledTooltipMilk);
                        UIWidgets::Tooltip("Cambia la leche de restaurar una cantidad fija de salud a restaurar un porcentaje de la salud máxima actual del jugador");

                        ImGui::Separator();

                        UIWidgets::EnhancementCheckbox("Efecto separado de media leche", "gSeparateHalfMilkEffect", disabledMilk, disabledTooltipMilk);
                        UIWidgets::Tooltip("Activa los siguientes cambios en la salud que restaura la media leche\nSi está desactivado, la media leche se comportará igual que la leche entera.");
                        bool disabledHalfMilk = disabledMilk || !CVarGetInteger("gSeparateHalfMilkEffect", 0);
                        const char* disabledTooltipHalfMilk = "Opción desactivada porque \"Efecto separado de media leche\" está desactivado";
                        UIWidgets::EnhancementSliderInt("Salud de media leche: %d", "##HALFMILKHEALTH", "gHalfMilkHealth", 1, 100, "", 0, true, disabledHalfMilk, disabledTooltipHalfMilk);
                        UIWidgets::Tooltip("Cambia la salud que restaura la media leche");
                        UIWidgets::EnhancementCheckbox("Media leche: restauración porcentual", "gHalfMilkPercentRestore", disabledHalfMilk, disabledTooltipHalfMilk);
                        UIWidgets::Tooltip("Cambia la media leche de restaurar una cantidad fija de salud a restaurar un porcentaje de la salud máxima actual del jugador");

                        ImGui::Separator();

                        UIWidgets::EnhancementCheckbox("Cambiar efecto de hada", "gFairyEffect");
                        UIWidgets::Tooltip("Activa los siguientes cambios en la salud que restauran las hadas");
                        bool disabledFairy = !CVarGetInteger("gFairyEffect", 0);
                        const char* disabledTooltipFairy = "Opción desactivada porque \"Cambiar efecto de hada\" está desactivado";
                        UIWidgets::EnhancementSliderInt("Hada: %d", "##FAIRYHEALTH", "gFairyHealth", 1, 100, "", 0, true, disabledFairy, disabledTooltipFairy);
                        UIWidgets::Tooltip("Cambia la salud que restauran las hadas");
                        UIWidgets::EnhancementCheckbox("Hada: restauración porcentual", "gFairyPercentRestore", disabledFairy, disabledTooltipFairy);
                        UIWidgets::Tooltip("Cambia las hadas de restaurar una cantidad fija de salud a restaurar un porcentaje de la salud máxima actual del jugador");

                        ImGui::Separator();

                        UIWidgets::EnhancementCheckbox("Cambiar efecto de revivir con hada", "gFairyReviveEffect");
                        UIWidgets::Tooltip("Activa los siguientes cambios en la salud que se restaura al revivir con un hada");
                        bool disabledFairyRevive = !CVarGetInteger("gFairyReviveEffect", 0);
                        const char* disabledTooltipFairyRevive = "Opción desactivada porque \"Cambiar efecto de revivir con hada\" está desactivado";
                        UIWidgets::EnhancementSliderInt("Revivir con hada: %d", "##FAIRYREVIVEHEALTH", "gFairyReviveHealth", 1, 100, "", 0, true, disabledFairyRevive, disabledTooltipFairyRevive);
                        UIWidgets::Tooltip("Cambia la salud que se restaura al revivir con un hada");
                        UIWidgets::EnhancementCheckbox("Revivir con hada: porcentual", "gFairyRevivePercentRestore", disabledFairyRevive, disabledTooltipFairyRevive);
                        UIWidgets::Tooltip("Cambia el revivir con hada de restaurar una cantidad fija de salud a restaurar un porcentaje de la salud máxima actual del jugador");

                        ImGui::EndMenu();
                    }

                    UIWidgets::Spacer(0);

                    if (ImGui::BeginMenu("Galería de tiro")) {
                        UIWidgets::EnhancementCheckbox("Personalizar comportamiento", "gCustomizeShootingGallery");
                        UIWidgets::Tooltip("Activa o desactiva los cambios en el comportamiento de la galería de tiro");
                        bool disabled = !CVarGetInteger("gCustomizeShootingGallery", 0);
                        const char* disabledTooltip = "Opción desactivada porque \"Personalizar comportamiento\" está desactivado";
                        UIWidgets::PaddedEnhancementCheckbox("Victoria instantánea", "gInstantShootingGalleryWin", true, false, disabled, disabledTooltip);
                        UIWidgets::Tooltip("Omite el minijuego de la galería de tiro");
                        UIWidgets::PaddedEnhancementCheckbox("Sin rupias aleatorias", "gConstantAdultGallery", true, false, disabled, disabledTooltip);
                        UIWidgets::Tooltip("Impide que el orden de las rupias sea aleatorio como adulto, haciéndolo igual que de niño");
                        UIWidgets::PaddedEnhancementSliderInt("Munición inicial (niño): %d", "##cShootingGalleryAmmunition", "gChildShootingGalleryAmmunition", 10, 30, "", 15, false, true, false, disabled, disabledTooltip);
                        UIWidgets::Tooltip("La munición al empezar el minijuego de la galería de tiro como niño");
                        UIWidgets::PaddedEnhancementSliderInt("Munición inicial (adulto): %d", "##aShootingGalleryAmmunition", "gAdultShootingGalleryAmmunition", 10, 30, "", 15, false, true, false, disabled, disabledTooltip);
                        UIWidgets::Tooltip("La munición al empezar el minijuego de la galería de tiro como adulto");
                        ImGui::EndMenu();
                    }

                    UIWidgets::Spacer(0);

                    if (ImGui::BeginMenu("Bolera de bombuchus")) {
                        UIWidgets::EnhancementCheckbox("Personalizar comportamiento", "gCustomizeBombchuBowling");
                        UIWidgets::Tooltip("Activa o desactiva los cambios en el comportamiento de la bolera de bombuchus");
                        bool disabled = CVarGetInteger("gCustomizeBombchuBowling", 0) == 0;
                        const char* disabledTooltip = "Opción desactivada porque \"Personalizar comportamiento\" está desactivado";
                        UIWidgets::PaddedEnhancementCheckbox("Quitar cuco pequeño", "gBombchuBowlingNoSmallCucco", true, false, disabled, disabledTooltip);
                        UIWidgets::Tooltip("Evita que aparezca el cuco pequeño en el minijuego de la bolera de bombuchus");
                        UIWidgets::PaddedEnhancementCheckbox("Quitar cuco grande", "gBombchuBowlingNoBigCucco", true, false, disabled, disabledTooltip);
                        UIWidgets::Tooltip("Evita que aparezca el cuco grande en el minijuego de la bolera de bombuchus");
                        UIWidgets::PaddedEnhancementSliderInt("Cantidad de bombuchus: %d", "##cBombchuBowlingAmmunition", "gBombchuBowlingAmmunition", 3, 20, "", 10, false, true, false, disabled, disabledTooltip);
                        UIWidgets::Tooltip("El número de bombuchus disponibles al empezar el minijuego de la bolera de bombuchus");
                        ImGui::EndMenu();
                    }

                    UIWidgets::Spacer(0);

                    if (ImGui::BeginMenu("Pesca")) {
                        UIWidgets::EnhancementCheckbox("Personalizar comportamiento", "gCustomizeFishing");
                        UIWidgets::Tooltip("Activa o desactiva los cambios en el comportamiento de la pesca");
                        bool disabled = !CVarGetInteger("gCustomizeFishing", 0);
                        const char* disabledTooltip = "Opción desactivada porque \"Personalizar comportamiento\" está desactivado";
                        UIWidgets::PaddedEnhancementCheckbox("Pesca instantánea", "gInstantFishing", true, false, disabled, disabledTooltip);
                        UIWidgets::Tooltip("Todos los peces se capturan al instante");
                        UIWidgets::PaddedEnhancementCheckbox("Picada garantizada", "gGuaranteeFishingBite", true, false, disabled, disabledTooltip);
                        UIWidgets::Tooltip("Cuando el sedal está estable, garantiza que piquen. Si no, se usa la lógica original");
                        UIWidgets::PaddedEnhancementCheckbox("Los peces no escapan", "gFishNeverEscape", true, false, disabled, disabledTooltip);
                        UIWidgets::Tooltip("Una vez clavado el anzuelo, el pez nunca se soltará mientras lo recoges.");
                        UIWidgets::PaddedEnhancementSliderInt("Peso mínimo (niño): %d", "##cMinimumWeight", "gChildMinimumWeightFish", 3, 10, "", 10, false, true, false, disabled, disabledTooltip);
                        UIWidgets::Tooltip("El peso mínimo para conseguir la recompensa única de pesca como niño");
                        UIWidgets::PaddedEnhancementSliderInt("Peso mínimo (adulto): %d", "##aMinimumWeight", "gAdultMinimumWeightFish", 6, 13, "", 13, false, true, false, disabled, disabledTooltip);
                        UIWidgets::Tooltip("El peso mínimo para conseguir la recompensa única de pesca como adulto");
                        ImGui::EndMenu();
                    }

                    UIWidgets::Spacer(0);

                    UIWidgets::PaddedEnhancementCheckbox("Modo rupias menguantes", "gRupeeDash", true, false);
                    UIWidgets::Tooltip("Las rupias disminuyen con el tiempo; Link sufre daño cuando llegan a 0.");
                    UIWidgets::PaddedEnhancementSliderInt("Intervalo de rupias menguantes: %d", "##DashInterval", "gDashInterval", 3, 5, "", 5, false, true, false,
                        !CVarGetInteger("gRupeeDash", 0), "Opción desactivada porque \"Modo rupias menguantes\" está desactivado");
                    UIWidgets::Tooltip("Intervalo entre reducciones de rupias en el modo rupias menguantes");

                    ImGui::EndMenu();
                }

                UIWidgets::Spacer(0);

                if (ImGui::BeginMenu("Interfaz limpia"))
                {
                    UIWidgets::EnhancementCheckbox("Silenciar alarma de poca vida", "gLowHpAlarm");
                    UIWidgets::Tooltip("Desactiva el pitido de poca vida");
                    UIWidgets::PaddedEnhancementCheckbox("Interfaz mínima", "gMinimalUI", true, false);
                    UIWidgets::Tooltip("Oculta la mayor parte de la interfaz cuando no hace falta\nNota: no se activa hasta cargar una escena nueva");
                    UIWidgets::PaddedEnhancementCheckbox("Sin voz de aviso de Navi", "gDisableNaviCallAudio", true, false);
                    UIWidgets::Tooltip("Desactiva el audio de voz cuando Navi te llama");

                    ImGui::EndMenu();
                }

                UIWidgets::Spacer(0);

                UIWidgets::EnhancementCheckbox("Piedra de la Agonía visual", "gVisualAgony");
                UIWidgets::Tooltip("Muestra un icono y reproduce un sonido cuando debería activarse la Piedra de la Agonía, para quienes no tienen vibración");
                UIWidgets::PaddedEnhancementCheckbox("Sayos y botas asignables", "gAssignableTunicsAndBoots", true, false);
                UIWidgets::Tooltip("Permite equipar los sayos y las botas en los botones C");
                UIWidgets::PaddedEnhancementCheckbox("Alternar equipamiento", "gEquipmentCanBeRemoved", true, false);
                UIWidgets::Tooltip("Permite quitar el equipamiento desactivándolo en\nla subpantalla de equipo.");
                UIWidgets::PaddedEnhancementCheckbox("Vaca de Link en ambas épocas", "gCowOfTime", true, false);
                UIWidgets::Tooltip("Permite compartir entre ambas épocas la recompensa de la carrera de obstáculos del Rancho Lon Lon");
                UIWidgets::PaddedEnhancementCheckbox("Mostrar visión de los guardias", "gGuardVision", true, false);
                UIWidgets::PaddedEnhancementCheckbox("Paso del tiempo al elegir archivo", "gTimeFlowFileSelect", true, false);
                UIWidgets::PaddedEnhancementCheckbox("Contadores en mensajes de objetos", "gInjectItemCounts", true, false);
                UIWidgets::Tooltip("Añade el número de objetos a los mensajes de recogida, como los símbolos de Skulltula Dorada y las piezas de corazón");
                UIWidgets::PaddedEnhancementCheckbox("Mover tumbas de día", "gDayGravePull", true, false);
                UIWidgets::Tooltip("Permite mover las tumbas de día siendo niño");
                UIWidgets::PaddedEnhancementCheckbox("Los perros te siguen a todas partes", "gDogFollowsEverywhere", true, false);
                UIWidgets::Tooltip("Permite que los perros te sigan adonde vayas, incluso si sales de la plaza del mercado");
                UIWidgets::PaddedEnhancementCheckbox("Créditos sin pulsar botones", "gNoInputForCredits", true, false);
                UIWidgets::Tooltip("Quita la necesidad de pulsar botones en los cuadros de texto tras derrotar a Ganon, permitiendo que avance la secuencia de créditos");

                // Blue Fire Arrows
                bool forceEnableBlueFireArrows = gSaveContext.n64ddFlag &&
                    OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_BLUE_FIRE_ARROWS);
                const char* forceEnableBlueFireArrowsText =
                    "Esta opción está activada a la fuerza porque hay una partida\ncargada con \"Flechas de fuego azul\".";
                UIWidgets::PaddedEnhancementCheckbox("Flechas de fuego azul", "gBlueFireArrows", true, false, 
                    forceEnableBlueFireArrows, forceEnableBlueFireArrowsText, UIWidgets::CheckboxGraphics::Checkmark);
                UIWidgets::Tooltip("Permite que las flechas de hielo derritan el hielo rojo.\nPuede requerir recargar la sala si se cambia durante la partida.");

                // Sunlight Arrows
                bool forceEnableSunLightArrows = gSaveContext.n64ddFlag &&
                    OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SUNLIGHT_ARROWS);
                const char* forceEnableSunLightArrowsText =
                    "Esta opción está activada a la fuerza porque hay una partida\ncargada con \"Flechas de luz solar\".";
                UIWidgets::PaddedEnhancementCheckbox("Flechas de luz solar", "gSunlightArrows", true, false, 
                    forceEnableSunLightArrows, forceEnableSunLightArrowsText, UIWidgets::CheckboxGraphics::Checkmark);
                UIWidgets::Tooltip("Permite que las flechas de luz activen los interruptores solares.\nPuede requerir recargar la sala si se cambia durante la partida.");

                UIWidgets::PaddedEnhancementCheckbox("Sin temblor de poca vida", "gDisableCritWiggle", true, false);
                UIWidgets::Tooltip("Desactiva el temblor aleatorio de la cámara con poca vida");

                ImGui::EndMenu();
            }

            UIWidgets::Spacer(0);

            if (ImGui::BeginMenu("Gráficos"))
            {
                if (ImGui::BeginMenu("Link animado en el menú de pausa")) {
                    ImGui::Text("Rotación");
                    UIWidgets::EnhancementRadioButton("Desactivado", "gPauseLiveLinkRotation", 0);
                    UIWidgets::EnhancementRadioButton("Rotar a Link con la cruceta", "gPauseLiveLinkRotation", 1);
                    UIWidgets::Tooltip("Permite rotar a Link en el menú de equipo con la cruceta\nUsa arriba o abajo en la cruceta para restablecer la rotación de Link");
                    UIWidgets::EnhancementRadioButton("Rotar a Link con los botones C", "gPauseLiveLinkRotation", 2);
                    UIWidgets::Tooltip("Permite rotar a Link en el menú de equipo con los botones C\nUsa C arriba o C abajo para restablecer la rotación de Link");
                    UIWidgets::EnhancementRadioButton("Rotar a Link con el stick derecho", "gPauseLiveLinkRotation", 3);
                    UIWidgets::Tooltip("Permite rotar a Link en el menú de equipo con el stick derecho\nPuedes acercar el zoom apuntando hacia arriba y restablecer la rotación de Link apuntando hacia abajo");
                    if (CVarGetInteger("gPauseLiveLinkRotation", 0) != 0) {
                        UIWidgets::EnhancementSliderInt("Velocidad de rotación: %d", "##MinRotationSpeed", "gPauseLiveLinkRotationSpeed", 1, 20, "");
                    }
                    UIWidgets::PaddedSeparator();
                    ImGui::Text("Bucle estático");
                    UIWidgets::EnhancementRadioButton("Desactivado", "gPauseLiveLink", 0);
                    UIWidgets::EnhancementRadioButton("Reposo (de pie)", "gPauseLiveLink", 1);
                    UIWidgets::EnhancementRadioButton("Reposo (mirando alrededor)", "gPauseLiveLink", 2);
                    UIWidgets::EnhancementRadioButton("Reposo (cinturón)", "gPauseLiveLink", 3);
                    UIWidgets::EnhancementRadioButton("Reposo (escudo)", "gPauseLiveLink", 4);
                    UIWidgets::EnhancementRadioButton("Reposo (probando la espada)", "gPauseLiveLink", 5);
                    UIWidgets::EnhancementRadioButton("Reposo (bostezo)", "gPauseLiveLink", 6);
                    UIWidgets::EnhancementRadioButton("Postura de combate", "gPauseLiveLink", 7);
                    UIWidgets::EnhancementRadioButton("Andando (sin escudo)", "gPauseLiveLink", 8);
                    UIWidgets::EnhancementRadioButton("Andando (con escudo)", "gPauseLiveLink", 9);
                    UIWidgets::EnhancementRadioButton("Corriendo (sin escudo)", "gPauseLiveLink", 10);
                    UIWidgets::EnhancementRadioButton("Corriendo (con escudo)", "gPauseLiveLink", 11);
                    UIWidgets::EnhancementRadioButton("Mano en la cadera", "gPauseLiveLink", 12);
                    UIWidgets::EnhancementRadioButton("Carga del ataque giratorio", "gPauseLiveLink", 13);
                    UIWidgets::EnhancementRadioButton("Mirar la mano", "gPauseLiveLink", 14);
                    UIWidgets::PaddedSeparator();
                    ImGui::Text("Aleatorizar");
                    UIWidgets::EnhancementRadioButton("Al azar", "gPauseLiveLink", 15);
                    UIWidgets::Tooltip("Elige una animación al azar cada vez que abres el menú");
                    UIWidgets::EnhancementRadioButton("Ciclo aleatorio", "gPauseLiveLink", 16);
                    UIWidgets::Tooltip("Cambia al azar la animación del menú pasado cierto tiempo");
                    UIWidgets::EnhancementRadioButton("Ciclo aleatorio (reposo)", "gPauseLiveLink", 17);
                    UIWidgets::Tooltip("Cambia al azar la animación del menú pasado cierto tiempo (solo animaciones de reposo)");
                    if (CVarGetInteger("gPauseLiveLink", 0) >= 16) {
                        UIWidgets::EnhancementSliderInt("Frames de espera: %d", "##MinFrameCount", "gMinFrameCount", 1, 1000, "", 0, true);
                    }

                    ImGui::EndMenu();
                }
                UIWidgets::PaddedEnhancementCheckbox("Modo N64", "gLowResMode", true, false);
                UIWidgets::Tooltip("Ajusta la relación de aspecto a 4:3 y baja la resolución a 240p, la resolución nativa de la N64");
                UIWidgets::PaddedEnhancementCheckbox("Marca para alinear glitches", "gDrawLineupTick", true, false);
                UIWidgets::Tooltip("Muestra una marca en el centro superior de la pantalla para ayudar con las alineaciones de glitches en SoH, ya que las alineaciones tradicionales basadas en la interfaz no funcionan fuera del formato 4:3");
                UIWidgets::PaddedEnhancementCheckbox("Objetos y proyectiles en 3D", "gNewDrops", true, false);
                UIWidgets::Tooltip("Cambia la mayoría de los objetos y proyectiles 2D del exterior por sus versiones en 3D");
                UIWidgets::PaddedEnhancementCheckbox("Sin bandas negras", "gDisableBlackBars", true, false);
                UIWidgets::Tooltip("Desactiva las bandas negras durante las escenas y al fijar blanco con Z\nNota: puede haber pequeños fallos visuales que las bandas negras tapaban\nDesactiva esta opción antes de informar de un error");
                UIWidgets::PaddedEnhancementCheckbox("Icono de bolsa dinámico", "gDynamicWalletIcon", true, false);
                UIWidgets::Tooltip("Cambia la rupia del icono de la bolsa para que coincida con el tamaño de bolsa que tienes");
                UIWidgets::PaddedEnhancementCheckbox("Mostrar siempre entradas de mazmorras", "gAlwaysShowDungeonMinimapIcon", true, false);
                UIWidgets::Tooltip("Muestra siempre los iconos de entrada de mazmorras en el minimapa");
                UIWidgets::PaddedText("Corregir caminos que desaparecen", true, false);
                const char* zFightingOptions[3] = { "Desactivado", "Desaparición uniforme", "Sin desaparición" };
                UIWidgets::EnhancementCombobox("gDirtPathFix", zFightingOptions, 3, 0);
                UIWidgets::Tooltip("Desactivado: los caminos desaparecen más cuanto mayor es la resolución (el Z-fighting depende de la resolución)\n"
                                   "Uniforme: ciertos caminos desaparecen igual en todas las resoluciones\n"
                                   "Sin desaparición: los caminos no desaparecen; Link parece hundirse en algunos\n"
                                   "Esto podría afectar a otros efectos de calcomanías (decals)\n");

                ImGui::EndMenu();
            }

            UIWidgets::Spacer(0);

            if (ImGui::BeginMenu("Correcciones"))
            {
                UIWidgets::EnhancementCheckbox("Corregir L y R en el menú de pausa", "gUniformLR");
                UIWidgets::Tooltip("Hace que los botones L y R del menú de pausa tengan el mismo color");
                UIWidgets::PaddedEnhancementCheckbox("Corregir cambio de página con L y Z", "gNGCKaleidoSwitcher", true, false);
                UIWidgets::Tooltip("Hace que L y R cambien de página como en GameCube\nZ abre el menú de depuración en su lugar");
                UIWidgets::PaddedEnhancementCheckbox("Corregir entradas de mazmorras", "gFixDungeonMinimapIcon", true, false);
                UIWidgets::Tooltip("Quita el icono de entrada de mazmorra de la esquina superior izquierda de la pantalla cuando no hay ninguna mazmorra en el mapa actual");
                UIWidgets::PaddedEnhancementCheckbox("Corregir reposo a dos manos", "gTwoHandedIdle", true, false);
                UIWidgets::Tooltip("Reactiva la animación de reposo a dos manos, una animación aparentemente terminada que se desactivó por accidente en el juego original");
                UIWidgets::PaddedEnhancementCheckbox("Corregir fallo de la gira de Dampé", "gGravediggingTourFix", true, false);
                UIWidgets::Tooltip("Corrige un error por el que la pieza de corazón de la gira de excavación de Dampé desaparece si se recarga la zona");
                UIWidgets::PaddedEnhancementCheckbox("Corregir mejora de nueces deku", "gDekuNutUpgradeFix", true, false);
                UIWidgets::Tooltip("Evita que la mejora de nueces deku del escenario del bosque deje de poder obtenerse tras recibir la Sierra del furtivo");
                UIWidgets::PaddedEnhancementCheckbox("Corregir posición del texto de Navi", "gNaviTextFix", true, false);
                UIWidgets::Tooltip("Centra correctamente el aviso de texto de Navi sobre el botón C arriba de la interfaz");
                UIWidgets::PaddedEnhancementCheckbox("Corregir bolas de fuego de Anubis", "gAnubisFix", true, false);
                UIWidgets::Tooltip("Hace que las bolas de fuego de Anubis causen daño de fuego al ser devueltas con el escudo espejo");
                UIWidgets::PaddedEnhancementCheckbox("Corregir estocada agachado (martillo)", "gCrouchStabHammerFix", true, false);
                UIWidgets::Tooltip("Hace que la estocada agachado del Martillo Megatón pueda destruir rocas sin haberlo blandido antes de forma normal");
                if (CVarGetInteger("gCrouchStabHammerFix", 0) == 0) {
                    CVarSetInteger("gCrouchStabFix", 0);
                } else {
                    UIWidgets::PaddedEnhancementCheckbox("Quitar estocada agachado potente", "gCrouchStabFix", true, false);
                    UIWidgets::Tooltip("Hace que la estocada agachado cause siempre el mismo daño que un tajo normal");
                }
                UIWidgets::PaddedEnhancementCheckbox("Corregir tiempos de los créditos", "gCreditsFix", true, false);
                UIWidgets::Tooltip("Alarga ciertas escenas de los créditos para que la música encaje con las imágenes");
                UIWidgets::PaddedEnhancementCheckbox("Corregir colores de la guerrera gerudo", "gGerudoWarriorClothingFix", true, false);
                UIWidgets::Tooltip("Evita que la ropa de la guerrera gerudo cambie de color al cambiar el sayo de Link o al usar bombas delante de ella");
                UIWidgets::PaddedEnhancementCheckbox("Corregir deriva de cámara", "gFixCameraDrift", true, false);
                UIWidgets::Tooltip("Corrige que la cámara se desvíe ligeramente a la izquierda cuando estás quieto por un error de cálculo");
                UIWidgets::PaddedEnhancementCheckbox("Corregir vaivén de cámara", "gFixCameraSwing", true, false);
                UIWidgets::Tooltip("Corrige que la cámara se atasque con las colisiones cuando estás quieto, y también el ligero retroceso de la cámara al dejar de moverte");
                UIWidgets::PaddedEnhancementCheckbox("Corregir giro al caer de un borde", "gFixHangingLedgeSwingRate", true, false);
                UIWidgets::Tooltip("Corrige la velocidad de giro de la cámara cuando el jugador cae de un saliente y la cámara gira alrededor");
                UIWidgets::PaddedEnhancementCheckbox("Corregir melodía tras 5 rupias de plata", "gSilverRupeeJingleExtend", true, false);
                UIWidgets::Tooltip(
                    "Añade 5 tonos más agudos a la melodía de las rupias de plata para las salas con más de 5 rupias de plata. "
                    "Actualmente solo es relevante en Master Quest.");

                ImGui::EndMenu();
            }

            UIWidgets::Spacer(0);

            if (ImGui::BeginMenu("Restauración"))
            {
                UIWidgets::EnhancementCheckbox("Sangre roja de Ganon", "gRedGanonBlood");
                UIWidgets::Tooltip("Restaura la sangre roja original de NTSC 1.0/1.1. Desactívalo para tener sangre verde");
                UIWidgets::PaddedEnhancementCheckbox("Pescar flotando", "gHoverFishing", true, false);
                UIWidgets::Tooltip("Restaura un error de NTSC 1.0 que permite lanzar la caña de pescar mientras se usan las botas voladoras");
                UIWidgets::PaddedEnhancementCheckbox("Frames raros de N64", "gN64WeirdFrames", true, false);
                UIWidgets::Tooltip("Restaura los frames raros de N64 para que los weirdshots se comporten igual que en N64");
                UIWidgets::PaddedEnhancementCheckbox("Bombuchus fuera de límites", "gBombchusOOB", true, false);
                UIWidgets::Tooltip("Permite que los bombuchus exploten fuera de los límites\nIgual que en GameCube y la Consola Virtual de Wii");
                UIWidgets::PaddedEnhancementCheckbox("Guardado rápido", "gQuickPutaway", true, false);
                UIWidgets::Tooltip("Restaura un error de NTSC 1.0 que permite guardar un objeto sin animación y hacer los \"Putaway Ocarina Items\"");
                UIWidgets::PaddedEnhancementCheckbox("Restaurar escena antigua de Skulltula Dorada", "gGsCutscene", true, false);
                UIWidgets::PaddedEnhancementCheckbox("Matar a Bongo Bongo rápido", "gQuickBongoKill", true, false);
                UIWidgets::Tooltip("Restaura un error de NTSC 1.0 que permite saltarse la escena de introducción de Bongo Bongo para matarlo rápidamente");

                ImGui::EndMenu();
            }

            UIWidgets::PaddedSeparator(false, true);

            // Autosave enum value of 1 is the default in presets and the old checkbox "on" state for backwards compatibility
            UIWidgets::PaddedText("Autoguardado", false, true);
            const char* autosaveLabels[] = { "Apagado", "Lugar nuevo + objeto importante", "Lugar nuevo + cualquier objeto", "Lugar nuevo", "Objeto importante", "Cualquier objeto" };
            UIWidgets::EnhancementCombobox("gAutosave", autosaveLabels, (sizeof(autosaveLabels) / sizeof(autosaveLabels[0])), CVarGetInteger("gAutosave", 0));
            UIWidgets::Tooltip("Guarda automáticamente la partida cada vez que entras en una zona nueva y/o consigues un objeto\n"
                "Los objetos importantes excluyen las rupias y las recargas de salud/magia/munición (pero incluyen los bombuchus, salvo que el botín de bombuchus esté activado)");

            UIWidgets::Spacer(0);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.38f, 0.56f, 1.0f));

            if (ImGui::Button(GetWindowButtonText("Personalizar controles", CVarGetInteger("gGameControlEditorEnabled", 0)).c_str(), ImVec2(-1.0f, 0.0f)))
            {
                bool currentValue = CVarGetInteger("gGameControlEditorEnabled", 0);
                CVarSetInteger("gGameControlEditorEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Game Control Editor", CVarGetInteger("gGameControlEditorEnabled", 0));
            }
            if (ImGui::Button(GetWindowButtonText("Editor de cosméticos", CVarGetInteger("gCosmeticsEditorEnabled", 0)).c_str(), ImVec2(-1.0f, 0.0f)))
            {
                bool currentValue = CVarGetInteger("gCosmeticsEditorEnabled", 0);
                CVarSetInteger("gCosmeticsEditorEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Cosmetics Editor", CVarGetInteger("gCosmeticsEditorEnabled", 0));
            }
            if (ImGui::Button(GetWindowButtonText("Editor de audio", CVarGetInteger("gAudioEditor.WindowOpen", 0)).c_str(), ImVec2(-1.0f, 0.0f)))
            {
                bool currentValue = CVarGetInteger("gAudioEditor.WindowOpen", 0);
                CVarSetInteger("gAudioEditor.WindowOpen", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Audio Editor", CVarGetInteger("gAudioEditor.WindowOpen", 0));
            }
            if (ImGui::Button(GetWindowButtonText("Estadísticas de juego", CVarGetInteger("gGameplayStatsEnabled", 0)).c_str(), ImVec2(-1.0f, 0.0f))) {
                bool currentValue = CVarGetInteger("gGameplayStatsEnabled", 0);
                CVarSetInteger("gGameplayStatsEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Gameplay Stats", CVarGetInteger("gGameplayStatsEnabled", 0));
            }
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(1);

            EXPERIMENTAL();

            const char* fps_cvar = "gInterpolationFPS";
            {
                int minFps = 20;
                int maxFps = Ship::Window::GetInstance()->GetCurrentRefreshRate();

                int val = OTRGlobals::Instance->GetInterpolationFPS();
                val = fmax(fmin(val, maxFps), 20);

            #ifdef __WIIU__
                // only support divisors of 60 on the Wii U
                val = 60 / (60 / val);
            #endif

                int fps = val;

                if (fps == 20)
                {
                    ImGui::Text("Interpolación de frames: No");
                }
                else
                {
                    ImGui::Text("Interpolación de frames: %d FPS", fps);
                }
                
                if (CVarGetInteger("gMatchRefreshRate", 0)) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }

                std::string MinusBTNFPSI = " - ##FPSInterpolation";
                std::string PlusBTNFPSI = " + ##FPSInterpolation";
                if (ImGui::Button(MinusBTNFPSI.c_str())) {
                #ifdef __WIIU__
                    if (val >= 60) val = 30;
                    else val = 20;
                #else
                    val--;
                #endif
                    CVarSetInteger(fps_cvar, val);
                    SohImGui::RequestCvarSaveOnNextTick();
                }
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 7.0f);
            #ifdef __SWITCH__
                ImGui::PushItemWidth(ImGui::GetWindowSize().x - 110.0f);
            #elif defined(__WIIU__)
                ImGui::PushItemWidth(ImGui::GetWindowSize().x - 79.0f * 2);
            #else
                ImGui::PushItemWidth(ImGui::GetWindowSize().x - 79.0f);
            #endif
                if (ImGui::SliderInt("##FPSInterpolation", &val, minFps, maxFps, "", ImGuiSliderFlags_AlwaysClamp))
                {
                #ifdef __WIIU__
                    // only support divisors of 60 on the Wii U
                    val = 60 / (60 / val);
                #endif
                    if (val > 360)
                    {
                        val = 360;
                    }
                    else if (val < 20)
                    {
                        val = 20;
                    }

                    CVarSetInteger(fps_cvar, val);
                    SohImGui::RequestCvarSaveOnNextTick();
                }
                ImGui::PopItemWidth();
                UIWidgets::Tooltip("Interpola frames adicionales para conseguir gráficos más fluidos\n"
                    "Ajústalo a la tasa de refresco de tu pantalla, o a un divisor de ella\n"
                    "Un objetivo de FPS superior a la tasa de refresco de tu pantalla solo malgastará recursos, "
                    "y podría dar un peor resultado.\n"
                    "Para un retardo de entrada constante, ajusta este valor y la tasa de refresco de tu pantalla a un múltiplo de 20\n"
                    "Ctrl+clic para escribir con el teclado");

                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 7.0f);
                if (ImGui::Button(PlusBTNFPSI.c_str())) {
                #ifdef __WIIU__
                    if (val <= 20) val = 30;
                    else val = 60;
                #else
                    val++;
                #endif
                    CVarSetInteger(fps_cvar, val);
                    SohImGui::RequestCvarSaveOnNextTick();
                }
                
                if (CVarGetInteger("gMatchRefreshRate", 0)) {
                    ImGui::PopItemFlag();
                    ImGui::PopStyleVar(1);
                }
            }
            
            UIWidgets::Spacer(0);
            UIWidgets::EnhancementCheckbox("Igualar tasa de refresco", "gMatchRefreshRate");
            UIWidgets::Tooltip("Iguala el valor de interpolación a la tasa de refresco de la ventana del juego");
            UIWidgets::Spacer(0);

            UIWidgets::EnhancementCheckbox("Desactivar LOD", "gDisableLOD");
            UIWidgets::Tooltip("Desactiva el nivel de detalle (LOD), de modo que los modelos usan sus variantes de más polígonos a cualquier distancia");
            if (UIWidgets::PaddedEnhancementCheckbox("Desactivar distancia de dibujado", "gDisableDrawDistance", true, false)) {
                if (CVarGetInteger("gDisableDrawDistance", 0) == 0) {
                    CVarSetInteger("gDisableKokiriDrawDistance", 0);
                }
            }
            UIWidgets::Tooltip("Desactiva la distancia de dibujado de los objetos, haciendo que sean visibles desde más lejos");
            if (CVarGetInteger("gDisableDrawDistance", 0) == 1) {
                UIWidgets::PaddedEnhancementCheckbox("Dibujado de Kokiri sin límite", "gDisableKokiriDrawDistance", true, false);
                UIWidgets::Tooltip("Los Kokiri son seres místicos que aparecen poco a poco al acercarte\nActivar esto eliminará su distancia de dibujado");
            }
            UIWidgets::PaddedEnhancementCheckbox("Omitir texto", "gSkipText", true, false);
            UIWidgets::Tooltip("Mantener B pulsado omite el texto");

         #ifdef __SWITCH__
            UIWidgets::Spacer(0);
            int slot = CVarGetInteger("gSwitchPerfMode", (int)Ship::SwitchProfiles::STOCK);
            ImGui::Text("Modo de rendimiento de Switch");
            if (ImGui::BeginCombo("##perf", SWITCH_CPU_PROFILES[slot])) {
                for (int sId = 0; sId <= Ship::SwitchProfiles::POWERSAVINGM3; sId++) {
                    if (ImGui::Selectable(SWITCH_CPU_PROFILES[sId], sId == slot)) {
                        SPDLOG_INFO("Profile:: %s", SWITCH_CPU_PROFILES[sId]);
                        CVarSetInteger("gSwitchPerfMode", sId);
                        Ship::Switch::ApplyOverclock();
                        SohImGui::RequestCvarSaveOnNextTick();
                    }

                }
                ImGui::EndCombo();
            }
         #endif

            ImGui::EndMenu();
        }

        ImGui::SetCursorPosY(0.0f);

        if (ImGui::BeginMenu("Trucos"))
        {
            if (ImGui::BeginMenu("Infinito...")) {
                UIWidgets::EnhancementCheckbox("Dinero", "gInfiniteMoney");
                UIWidgets::PaddedEnhancementCheckbox("Salud", "gInfiniteHealth", true, false);
                UIWidgets::PaddedEnhancementCheckbox("Munición", "gInfiniteAmmo", true, false);
                UIWidgets::PaddedEnhancementCheckbox("Magia", "gInfiniteMagic", true, false);
                UIWidgets::PaddedEnhancementCheckbox("Amor de Nayru", "gInfiniteNayru", true, false);
                UIWidgets::PaddedEnhancementCheckbox("Impulso de Epona", "gInfiniteEpona", true, false);

                ImGui::EndMenu();
            }

            UIWidgets::PaddedEnhancementCheckbox("Atravesar paredes", "gNoClip", true, false);
            UIWidgets::Tooltip("Te permite atravesar las paredes");
            UIWidgets::PaddedEnhancementCheckbox("Escalar todo", "gClimbEverything", true, false);
            UIWidgets::Tooltip("Hace que se pueda escalar cualquier superficie del juego");
            UIWidgets::PaddedEnhancementCheckbox("Gancho en todo", "gHookshotEverything", true, false);
            UIWidgets::Tooltip("Hace que se pueda enganchar el gancho en cualquier superficie del juego");
            UIWidgets::EnhancementSliderFloat("Multiplicador de alcance del gancho: %.1fx", "##gCheatHookshotReachMultiplier", "gCheatHookshotReachMultiplier", 1.0f, 5.0f, "", 1.0f, false);
            UIWidgets::PaddedEnhancementCheckbox("Salto lunar con L", "gMoonJumpOnL", true, false);
            UIWidgets::Tooltip("Mantener L te hace flotar en el aire");
            UIWidgets::PaddedEnhancementCheckbox("Super sayo", "gSuperTunic", true, false);
            UIWidgets::Tooltip("Hace que cada sayo tenga los efectos de todos los demás");
            UIWidgets::PaddedEnhancementCheckbox("ISG fácil", "gEzISG", true, false);
            UIWidgets::Tooltip("Glitch de la espada infinita (ISG) pasivo\nHace que el efecto del tajo y la hitbox de tu espada permanezcan activos indefinidamente");
            UIWidgets::PaddedEnhancementCheckbox("Equipamiento atemporal", "gTimelessEquipment", true, false);
            UIWidgets::Tooltip("Permite equipar cualquier objeto sin importar la edad\nTambién permite al Link niño usar las mejoras de fuerza del adulto");
            UIWidgets::PaddedEnhancementCheckbox("Avance de frames fácil", "gCheatEasyPauseBufferEnabled", true, false);
            UIWidgets::Tooltip("Mantén pulsado START al salir de la pausa para avanzar un solo frame y volver a pausar");
            const bool bEasyFrameAdvanceEnabled = CVarGetInteger("gCheatEasyPauseBufferEnabled", 0);
            UIWidgets::PaddedEnhancementCheckbox("Búfer de entrada fácil", "gCheatEasyInputBufferingEnabled", true, false, bEasyFrameAdvanceEnabled, "Se activa a la fuerza cuando el avance de frames fácil está activado");
            UIWidgets::Tooltip("Los botones que mantengas pulsados mientras se cierra la subpantalla se pulsarán al reanudar el juego");
            UIWidgets::PaddedEnhancementCheckbox("Objetos sin restricciones", "gNoRestrictItems", true, false);
            UIWidgets::Tooltip("Permite usar cualquier objeto en cualquier lugar");
            UIWidgets::PaddedEnhancementCheckbox("Congelar tiempo", "gFreezeTime", true, false);
            UIWidgets::Tooltip("Congela la hora del día");
            UIWidgets::PaddedEnhancementCheckbox("Botín permanente", "gDropsDontDie", true, false);
            UIWidgets::Tooltip("El botín de los enemigos, la hierba, etc. no desaparece pasado un tiempo");
            UIWidgets::PaddedEnhancementCheckbox("Escudo deku ignífugo", "gFireproofDekuShield", true, false);
            UIWidgets::Tooltip("Evita que el escudo deku se queme al contacto con el fuego");
            UIWidgets::PaddedEnhancementCheckbox("Escudo con armas a dos manos", "gShieldTwoHanded", true, false);
            UIWidgets::Tooltip("Permite alzar el escudo con cualquier arma a dos manos, excepto con los palos deku");
            UIWidgets::PaddedEnhancementCheckbox("Sincronizar hora", "gTimeSync", true, false);
            UIWidgets::Tooltip("Sincroniza la hora del juego con la hora real");

            {
                static int32_t betaQuestEnabled = CVarGetInteger("gEnableBetaQuest", 0);
                static int32_t lastBetaQuestEnabled = betaQuestEnabled;
                static int32_t betaQuestWorld = CVarGetInteger("gBetaQuestWorld", 0xFFEF);
                static int32_t lastBetaQuestWorld = betaQuestWorld;

                if (!isBetaQuestEnabled) {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                }

                UIWidgets::PaddedEnhancementCheckbox("Activar Beta Quest", "gEnableBetaQuest", true, false);
                UIWidgets::Tooltip("Activa la Beta Quest de OoT. *AVISO* Se reiniciará el juego.");
                betaQuestEnabled = CVarGetInteger("gEnableBetaQuest", 0);
                if (betaQuestEnabled) {
                    if (betaQuestEnabled != lastBetaQuestEnabled) {
                        betaQuestWorld = 0;
                    }

                    ImGui::Text("Mundo de Beta Quest: %d", betaQuestWorld);

                    if (ImGui::Button(" - ##BetaQuest")) {
                        betaQuestWorld--;
                    }
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 7.0f);

                    ImGui::SliderInt("##BetaQuest", &betaQuestWorld, 0, 8, "", ImGuiSliderFlags_AlwaysClamp);
                    UIWidgets::Tooltip("Elige el mundo de la Beta Quest que explorar. *AVISO* Cambiarlo reiniciará el juego.\nCtrl+clic para escribir un valor.");

                    ImGui::SameLine();
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 7.0f);
                    if (ImGui::Button(" + ##BetaQuest")) {
                        betaQuestWorld++;
                    }

                    if (betaQuestWorld > 8) {
                        betaQuestWorld = 8;
                    }
                    else if (betaQuestWorld < 0) {
                        betaQuestWorld = 0;
                    }
                }
                else {
                    lastBetaQuestWorld = betaQuestWorld = 0xFFEF;
                    CVarSetInteger("gBetaQuestWorld", betaQuestWorld);
                }
                if (betaQuestEnabled != lastBetaQuestEnabled || betaQuestWorld != lastBetaQuestWorld)
                {
                    // Reset the game if the beta quest state or world changed because beta quest happens on redirecting the title screen cutscene.
                    lastBetaQuestEnabled = betaQuestEnabled;
                    lastBetaQuestWorld = betaQuestWorld;
                    CVarSetInteger("gEnableBetaQuest", betaQuestEnabled);
                    CVarSetInteger("gBetaQuestWorld", betaQuestWorld);

                    SohImGui::DispatchConsoleCommand("reset");

                    SohImGui::RequestCvarSaveOnNextTick();
                }

                if (!isBetaQuestEnabled) {
                    ImGui::PopItemFlag();
                    ImGui::PopStyleVar(1);
                }
            }

            if (ImGui::Button("Cambiar edad")) {
                CVarSetInteger("gSwitchAge", 1);
            }
            UIWidgets::Tooltip("Cambia la edad de Link y recarga la zona.");   

            ImGui::EndMenu();
        }

        ImGui::SetCursorPosY(0.0f);

        if (ImGui::BeginMenu("Desarrollo"))
        {
            UIWidgets::EnhancementCheckbox("Modo depuración de OoT", "gDebugEnabled");
            UIWidgets::Tooltip("Activa el modo de depuración: permite elegir mapas con L + R + Z, atravesar paredes con L + cruceta derecha y abrir el menú de depuración con L en la pantalla de pausa");
            UIWidgets::PaddedEnhancementCheckbox("Depuración de Skulltulas", "gSkulltulaDebugEnabled", true, false);
            UIWidgets::Tooltip("Activa la depuración de Skulltulas: al mover el cursor del menú sobre varios iconos del mapa (gran llave, brújula, ubicaciones de la pantalla del mapa, etc.) se marcarán los bits de GS de esa zona.\nÚSALO CON CUIDADO, YA QUE NO ACTUALIZA EL CONTADOR DE GS.");
            UIWidgets::PaddedEnhancementCheckbox("Selección rápida de archivo", "gSkipLogoTitle", true, false);
            UIWidgets::Tooltip("Carga el juego en el menú o archivo seleccionado\n\"Selector de mapas de Zelda\" requiere el modo de depuración; si no, volverás al menú de selección de archivo\nSi eliges un archivo sin partida, solo se creará una partida nueva si activas \"¿Crear partida nueva si no hay ninguna?\"; si no, irás al menú de selección de archivo");
            if (CVarGetInteger("gSkipLogoTitle", 0)) {
                const char* FastFileSelect[5] = {
                    "Archivo n.º 1",
                    "Archivo n.º 2",
                    "Archivo n.º 3",
                    "Selector de mapas (requiere modo depuración)",
                    "Selección de archivo",
                };
                ImGui::Text("Cargar:");
                UIWidgets::EnhancementCombobox("gSaveFileID", FastFileSelect, 5, 0);
            };
            UIWidgets::PaddedEnhancementCheckbox("Ocultar info de versión", "gHideBuildInfo", true, false);
            UIWidgets::Tooltip("Oculta la versión del juego y los detalles de la compilación en la pantalla inicial con el logo");
            UIWidgets::PaddedEnhancementCheckbox("Pantalla de salto mejorada", "gBetterDebugWarpScreen", true, false);
            UIWidgets::Tooltip("Pantalla de salto de depuración optimizada, con la posibilidad añadida de elegir entradas y hora del día");
            UIWidgets::PaddedEnhancementCheckbox("Traducir pantalla de salto", "gDebugWarpScreenTranslation", true, false);
            UIWidgets::Tooltip("Traduce la pantalla de salto de depuración según el idioma del juego");
            UIWidgets::PaddedSeparator();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0,0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.38f, 0.56f, 1.0f));
            if (ImGui::Button(GetWindowButtonText("Estadísticas", CVarGetInteger("gStatsEnabled", 0)).c_str(), ImVec2(-1.0f, 0.0f)))
            {
                bool currentValue = CVarGetInteger("gStatsEnabled", 0);
                CVarSetInteger("gStatsEnabled", !currentValue);
                SohImGui::ToggleStatisticsWindow(true);
                SohImGui::RequestCvarSaveOnNextTick();
            }
            UIWidgets::Tooltip("Muestra la ventana de estadísticas, con tus FPS, los tiempos de frame y el sistema operativo en el que juegas");
            UIWidgets::Spacer(0);
            if (ImGui::Button(GetWindowButtonText("Consola", CVarGetInteger("gConsoleEnabled", 0)).c_str(), ImVec2(-1.0f, 0.0f)))
            {
                bool currentValue = CVarGetInteger("gConsoleEnabled", 0);
                CVarSetInteger("gConsoleEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::ToggleConsoleWindow(!currentValue);
            }
            UIWidgets::Tooltip("Activa la ventana de consola, que permite introducir comandos; escribe help para ver algunos ejemplos");
            UIWidgets::Spacer(0);
            if (ImGui::Button(GetWindowButtonText("Editor de partida", CVarGetInteger("gSaveEditorEnabled", 0)).c_str(), ImVec2(-1.0f, 0.0f)))
            {
                bool currentValue = CVarGetInteger("gSaveEditorEnabled", 0);
                CVarSetInteger("gSaveEditorEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Save Editor", CVarGetInteger("gSaveEditorEnabled", 0));
            }
            UIWidgets::Spacer(0);
            if (ImGui::Button(GetWindowButtonText("Visor de colisiones", CVarGetInteger("gCollisionViewerEnabled", 0)).c_str(), ImVec2(-1.0f, 0.0f)))
            {
                bool currentValue = CVarGetInteger("gCollisionViewerEnabled", 0);
                CVarSetInteger("gCollisionViewerEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Collision Viewer", CVarGetInteger("gCollisionViewerEnabled", 0));
            }
            UIWidgets::Spacer(0);
            if (ImGui::Button(GetWindowButtonText("Visor de actores", CVarGetInteger("gActorViewerEnabled", 0)).c_str(), ImVec2(-1.0f, 0.0f)))
            {
                bool currentValue = CVarGetInteger("gActorViewerEnabled", 0);
                CVarSetInteger("gActorViewerEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Actor Viewer", CVarGetInteger("gActorViewerEnabled", 0));
            }
            UIWidgets::Spacer(0);
            if (ImGui::Button(GetWindowButtonText("Visor de display lists", CVarGetInteger("gDLViewerEnabled", 0)).c_str(), ImVec2(-1.0f, 0.0f)))
            {
                bool currentValue = CVarGetInteger("gDLViewerEnabled", 0);
                CVarSetInteger("gDLViewerEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Display List Viewer", CVarGetInteger("gDLViewerEnabled", 0));
            }
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(1);

            ImGui::EndMenu();
        }

        ImGui::SetCursorPosY(0.0f);

        if (ImGui::BeginMenu("Randomizer"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.38f, 0.56f, 1.0f));
        #ifdef __WIIU__
            static ImVec2 buttonSize(200.0f * 2.0f, 0.0f);
        #else
            static ImVec2 buttonSize(200.0f, 0.0f);
        #endif
            if (ImGui::Button(GetWindowButtonText("Ajustes del Randomizer", CVarGetInteger("gRandomizerSettingsEnabled", 0)).c_str(), buttonSize))
            {
                bool currentValue = CVarGetInteger("gRandomizerSettingsEnabled", 0);
                CVarSetInteger("gRandomizerSettingsEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Randomizer Settings", CVarGetInteger("gRandomizerSettingsEnabled", 0));
            }
            ImGui::Dummy(ImVec2(0.0f, 0.0f));
            if (ImGui::Button(GetWindowButtonText("Rastreador de objetos", CVarGetInteger("gItemTrackerEnabled", 0)).c_str(), buttonSize))
            {
                bool currentValue = CVarGetInteger("gItemTrackerEnabled", 0);
                CVarSetInteger("gItemTrackerEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Item Tracker", CVarGetInteger("gItemTrackerEnabled", 0));
            }
            ImGui::Dummy(ImVec2(0.0f, 0.0f));
            if (ImGui::Button(GetWindowButtonText("Ajust. rastreador objetos", CVarGetInteger("gItemTrackerSettingsEnabled", 0)).c_str(), buttonSize))
            {
                bool currentValue = CVarGetInteger("gItemTrackerSettingsEnabled", 0);
                CVarSetInteger("gItemTrackerSettingsEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Item Tracker Settings", CVarGetInteger("gItemTrackerSettingsEnabled", 0));
            }
            ImGui::Dummy(ImVec2(0.0f, 0.0f));
            if (ImGui::Button(GetWindowButtonText("Rastreador de entradas", CVarGetInteger("gEntranceTrackerEnabled", 0)).c_str(), buttonSize))
            {
                bool currentValue = CVarGetInteger("gEntranceTrackerEnabled", 0);
                CVarSetInteger("gEntranceTrackerEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Entrance Tracker", CVarGetInteger("gEntranceTrackerEnabled", 0));
            }
            ImGui::Dummy(ImVec2(0.0f, 0.0f));
            if (ImGui::Button(GetWindowButtonText("Rastreador de ubicaciones", CVarGetInteger("gCheckTrackerEnabled", 0)).c_str(), buttonSize))
            {
                bool currentValue = CVarGetInteger("gCheckTrackerEnabled", 0);
                CVarSetInteger("gCheckTrackerEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Check Tracker", CVarGetInteger("gCheckTrackerEnabled", 0));
            }
            ImGui::Dummy(ImVec2(0.0f, 0.0f));
            if (ImGui::Button(GetWindowButtonText("Ajust. rastreador ubicaciones", CVarGetInteger("gCheckTrackerSettingsEnabled", 0)).c_str(), buttonSize))
            {
                bool currentValue = CVarGetInteger("gCheckTrackerSettingsEnabled", 0);
                CVarSetInteger("gCheckTrackerSettingsEnabled", !currentValue);
                SohImGui::RequestCvarSaveOnNextTick();
                SohImGui::EnableWindow("Check Tracker Settings", CVarGetInteger("gCheckTrackerSettingsEnabled", 0));
            }
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(1);

            UIWidgets::PaddedSeparator();

            if (ImGui::BeginMenu("Mejoras del Randomizer"))
            {
                UIWidgets::EnhancementCheckbox("Pistas de Navi para el rando", "gRandoRelevantNavi");
                UIWidgets::Tooltip(
                    "Sustituye las pistas de misión de Navi en el exterior por pistas de juego relacionadas con el randomizer."
                );
                UIWidgets::PaddedEnhancementCheckbox("Nombres de rupias aleatorios", "gRandomizeRupeeNames", true, false);
                UIWidgets::Tooltip(
                    "Al conseguir rupias, el nombre de la rupia en el cuadro de texto es aleatorio."
                );

                // Only disable the key colors checkbox when none of the keysanity settings are set to "Any Dungeon", "Overworld" or "Anywhere"
                bool disableKeyColors = true;

                if (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_KEYSANITY) == RO_DUNGEON_ITEM_LOC_ANY_DUNGEON ||
                    OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_KEYSANITY) == RO_DUNGEON_ITEM_LOC_OVERWORLD ||
                    OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_KEYSANITY) == RO_DUNGEON_ITEM_LOC_ANYWHERE ||
                    OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_GERUDO_KEYS) != RO_GERUDO_KEYS_VANILLA ||
                    OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_BOSS_KEYSANITY) == RO_DUNGEON_ITEM_LOC_ANY_DUNGEON ||
                    OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_BOSS_KEYSANITY) == RO_DUNGEON_ITEM_LOC_OVERWORLD ||
                    OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_BOSS_KEYSANITY) == RO_DUNGEON_ITEM_LOC_ANYWHERE ||
                    (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_GANONS_BOSS_KEY) != RO_GANON_BOSS_KEY_VANILLA &&
                     OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_GANONS_BOSS_KEY) != RO_GANON_BOSS_KEY_OWN_DUNGEON &&
                     OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_GANONS_BOSS_KEY) != RO_GANON_BOSS_KEY_STARTWITH) || 
                    !gSaveContext.n64ddFlag) {
                    disableKeyColors = false;
                }

                const char* disableKeyColorsText = 
                    "Esta opción está desactivada porque hay una partida cargada sin ningún\n"
                    "ajuste de llaves aleatorias en \"Cualquier mazmorra\", \"Exterior\" o \"Cualquier lugar\"";

                UIWidgets::PaddedEnhancementCheckbox("Colores de llaves según mazmorra", "gRandoMatchKeyColors", true, false,
                                                     disableKeyColors, disableKeyColorsText);
                UIWidgets::Tooltip(
                    "Hace coincidir el color de las llaves pequeñas y las grandes llaves con el de la mazmorra a la que pertenecen. "
                    "Ayuda a identificar las llaves desde lejos y añade un toque de estilo.\n\nSolo "
                    "se aplica a seeds con llaves y grandes llaves mezcladas en cualquier mazmorra, en el exterior o en cualquier lugar.");
                UIWidgets::PaddedEnhancementCheckbox("Fanfarrias de objetos de misión", "gRandoQuestItemFanfares", true, false);
                UIWidgets::Tooltip(
                    "Reproduce fanfarrias exclusivas al obtener objetos de misión "
                    "(medallones, piedras y canciones). Ten en cuenta que estas fanfarrias son más largas de lo normal."
                );
                ImGui::EndMenu();
            }

            UIWidgets::PaddedSeparator();

        #ifdef ENABLE_CROWD_CONTROL
            UIWidgets::EnhancementCheckbox("Crowd Control", "gCrowdControl");
            UIWidgets::Tooltip("Intentará conectarse al servidor de Crowd Control. Visita crowdcontrol.live para más información.");

            if (CVarGetInteger("gCrowdControl", 0)) {
                CrowdControl::Instance->Enable();
            } else {
                CrowdControl::Instance->Disable();
            }

            ImGui::Dummy(ImVec2(0.0f, 0.0f));
        #endif

            UIWidgets::EnhancementCheckbox("Randomizer de enemigos", "gRandomizedEnemies");
            UIWidgets::Tooltip(
                "Aleatoriza los enemigos normales cada vez que cargas una sala. Se excluyen los jefes, los minijefes y algunos enemigos normales concretos.\n\n"
                "Los enemigos que necesitan algo más que nueces deku + palos deku o una espada para morir no aparecen en las salas que se abren al derrotar a todos los enemigos."
            );

            if (CVarGetInteger("gRandomizedEnemies", 0)) {

                bool disableSeededEnemies = !gSaveContext.n64ddFlag && gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2;
                const char* disableSeededEnemiesText = "Esta opción está desactivada porque depende de una partida randomizer.";

                UIWidgets::PaddedEnhancementCheckbox("Enemigos según la seed", "gSeededRandomizedEnemies", true, false, disableSeededEnemies, disableSeededEnemiesText);
                UIWidgets::Tooltip(
                    "Los enemigos se mantendrán iguales al recargar las salas. Se basan en las seeds del randomizer, por lo que solo funciona con partidas randomizer."
                );
            }

            ImGui::EndMenu();
        }
    }
}
