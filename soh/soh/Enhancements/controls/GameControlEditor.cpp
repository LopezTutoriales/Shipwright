#include "GameControlEditor.h"

#include <string>
#include <list>
#include <unordered_map>
#include <utility>
#include <iterator>
#include <variables.h>

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>
#include <libultraship/bridge.h>
#include <libultraship/libultra/controller.h>
#include <Utils/StringHelper.h>
#include <ImGuiImpl.h>

#include "../../UIWidgets.hpp"

namespace GameControlEditor {
    const ImGuiTableFlags PANEL_TABLE_FLAGS =
        ImGuiTableFlags_BordersH |
        ImGuiTableFlags_BordersV;
    const ImGuiTableColumnFlags PANEL_TABLE_COLUMN_FLAGS =
        ImGuiTableColumnFlags_IndentEnable |
        ImGuiTableColumnFlags_NoSort;

    namespace TableHelper {
        void InitHeader(bool has_header = true) {
            if (has_header) {
                ImGui::TableHeadersRow();
            }
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding(); //This is to adjust Vertical pos of item in a cell to be normlized.
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        }

        void NextCol() {
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        }

        void NextLine() {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        }
    }

    void DrawHelpIcon(const std::string& helptext) {
        // place the ? button to the most of the right side of the cell it is using.
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 22);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 15);
        ImGui::SmallButton("?");
        UIWidgets::Tooltip(helptext.c_str());
    }

    typedef uint32_t N64ButtonMask;

    // Used together for an incomplete linked hash map implementation in order to
    // map button masks to their names and original mapping on N64
    static std::list<std::pair<N64ButtonMask, const char*>> buttons;
    static std::unordered_map<N64ButtonMask, decltype(buttons)::iterator> buttonNames;

    void addButtonName(N64ButtonMask mask, const char* name) {
        buttons.push_back(std::make_pair(mask, name));
        buttonNames[mask] = std::prev(buttons.end());
    }

    typedef struct {
        const char* label;
        const char* cVarName;
        N64ButtonMask defaultBtn;
    } CustomButtonMap;

    // Ocarina button maps
    static CustomButtonMap ocarinaD5 = {"D5", "gOcarinaD5BtnMap", BTN_CUP};
    static CustomButtonMap ocarinaB4 = {"B4", "gOcarinaB4BtnMap", BTN_CLEFT};
    static CustomButtonMap ocarinaA4 = {"A4", "gOcarinaA4BtnMap", BTN_CRIGHT};
    static CustomButtonMap ocarinaF4 = {"F4", "gOcarinaF4BtnMap", BTN_CDOWN};
    static CustomButtonMap ocarinaD4 = {"D4", "gOcarinaD4BtnMap", BTN_A};
    static CustomButtonMap ocarinaSongDisable = {"Desactivar canciones", "gOcarinaDisableBtnMap", BTN_L};
    static CustomButtonMap ocarinaSharp = {"Subir tono", "gOcarinaSharpBtnMap", BTN_R};
    static CustomButtonMap ocarinaFlat = {"Bajar tono", "gOcarinaFlatBtnMap", BTN_Z};

    void DrawUI(bool&);

    void Init() {
        SohImGui::AddWindow("Enhancements", "Game Control Editor", DrawUI);

        addButtonName(BTN_A,		"A");
        addButtonName(BTN_B,		"B");
        addButtonName(BTN_CUP,		"C arriba");
        addButtonName(BTN_CDOWN,	"C abajo");
        addButtonName(BTN_CLEFT,	"C izquierda");
        addButtonName(BTN_CRIGHT,	"C derecha");
        addButtonName(BTN_L,		"L");
        addButtonName(BTN_Z,		"Z");
        addButtonName(BTN_R,		"R");
        addButtonName(BTN_START,	"Start");
        addButtonName(BTN_DUP,		"Cruceta arriba");
        addButtonName(BTN_DDOWN,	"Cruceta abajo");
        addButtonName(BTN_DLEFT,	"Cruceta izquierda");
        addButtonName(BTN_DRIGHT,	"Cruceta derecha");
        addButtonName(0,			"Ninguno");
    }

    // Draw a button mapping setting consisting of a padded label and button dropdown.
    // excludedButtons indicates which buttons are unavailable to choose from.
    void DrawMapping(CustomButtonMap& mapping, float labelWidth, N64ButtonMask excludedButtons) {
        N64ButtonMask currentButton = CVarGetInteger(mapping.cVarName, mapping.defaultBtn);

        const char* preview;
        if (buttonNames.contains(currentButton)) {
            preview = buttonNames[currentButton]->second;
        } else {
            preview = "Desconocido";
        }

        UIWidgets::Spacer(0);
        ImVec2 cursorPos = ImGui::GetCursorPos();
        ImVec2 textSize = ImGui::CalcTextSize(mapping.label);
        ImGui::SetCursorPosY(cursorPos.y + textSize.y / 4);
        ImGui::SetCursorPosX(cursorPos.x + abs(textSize.x - labelWidth));
        ImGui::Text("%s", mapping.label);
        ImGui::SameLine();
        ImGui::SetCursorPosY(cursorPos.y);

        ImGui::SetNextItemWidth(ImGui::GetFontSize() * 8);
            if (ImGui::BeginCombo(StringHelper::Sprintf("##%s", mapping.cVarName).c_str(), preview)) {
            for (auto i = buttons.begin(); i != buttons.end(); i++) {
                if ((i->first & excludedButtons) != 0) {
                    continue;
                }
                if (ImGui::Selectable(i->second, i->first == currentButton)) {
                    CVarSetInteger(mapping.cVarName, i->first);
                }
            }
            ImGui::EndCombo();
        }
        UIWidgets::Spacer(0);
    }

    void DrawOcarinaControlPanel() {
        if (!ImGui::CollapsingHeader("Controles de la ocarina")) {
            return;
        }

        if (!ImGui::BeginTable("tableCustomOcarinaControls", 1, PANEL_TABLE_FLAGS)) {
            return;
        }

        ImGui::TableSetupColumn("Controles personalizados de la ocarina", PANEL_TABLE_COLUMN_FLAGS | ImGuiTableColumnFlags_WidthStretch);
        TableHelper::InitHeader(false);
        
        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cursor.x + 5, cursor.y + 5));
        UIWidgets::EnhancementCheckbox("Personalizar controles de la ocarina", "gCustomOcarinaControls");

        if (CVarGetInteger("gCustomOcarinaControls", 0) == 1) {
            if (ImGui::BeginTable("tableCustomMainOcarinaControls", 2, ImGuiTableFlags_SizingStretchProp)) {
                float labelWidth;
                N64ButtonMask disableMask = BTN_B;
                if (CVarGetInteger("gDpadOcarina", 0)) {
                    disableMask |= BTN_DUP | BTN_DDOWN | BTN_DLEFT | BTN_DRIGHT;
                }

                ImGui::TableSetupColumn("Notas##CustomOcarinaNotes", PANEL_TABLE_COLUMN_FLAGS);
                ImGui::TableSetupColumn("Modificadores##CustomOcaranaModifiers", PANEL_TABLE_COLUMN_FLAGS);
                TableHelper::InitHeader(false);

                SohImGui::BeginGroupPanel("Notas", ImGui::GetContentRegionAvail());
                labelWidth = ImGui::CalcTextSize("D5").x + 10;
                DrawMapping(ocarinaD5, labelWidth, disableMask);
                DrawMapping(ocarinaB4, labelWidth, disableMask);
                DrawMapping(ocarinaA4, labelWidth, disableMask);
                DrawMapping(ocarinaF4, labelWidth, disableMask);
                DrawMapping(ocarinaD4, labelWidth, disableMask);
                ImGui::Dummy(ImVec2(0, 5));
                float cursorY = ImGui::GetCursorPosY();
                SohImGui::EndGroupPanel();

                TableHelper::NextCol();

                SohImGui::BeginGroupPanel("Modificadores", ImGui::GetContentRegionAvail());
                labelWidth = ImGui::CalcTextSize(ocarinaSongDisable.label).x + 10;
                DrawMapping(ocarinaSongDisable, labelWidth, disableMask);
                DrawMapping(ocarinaSharp, labelWidth, disableMask);
                DrawMapping(ocarinaFlat, labelWidth, disableMask);
                SohImGui::EndGroupPanel(cursorY - ImGui::GetCursorPosY() + 2);

                ImGui::EndTable();
            }
        } else {
            UIWidgets::Spacer(0);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);
            ImGui::TextWrapped("Para modificar los controles principales de la ocarina, marca la casilla \"Personalizar controles de la ocarina\".");
            UIWidgets::Spacer(0);
        }

        SohImGui::BeginGroupPanel("Controles alternativos", ImGui::GetContentRegionAvail());
        if (ImGui::BeginTable("tableOcarinaAlternateControls", 2, ImGuiTableFlags_SizingFixedSame)) {
            ImGui::TableSetupColumn("Cruceta", PANEL_TABLE_COLUMN_FLAGS);
            ImGui::TableSetupColumn("Stick derecho", PANEL_TABLE_COLUMN_FLAGS);
            TableHelper::InitHeader(false);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);
            UIWidgets::EnhancementCheckbox("Tocar con la cruceta", "gDpadOcarina");
            TableHelper::NextCol();
            UIWidgets::EnhancementCheckbox("Tocar con el stick de cámara", "gRStickOcarina");
            ImGui::EndTable();
        }
        SohImGui::EndGroupPanel();

        ImGui::EndTable();
    }

    // CurrentPort is indexed started at 1 here due to the Generic tab, instead of 0 like in InputEditor
    // Therefore CurrentPort - 1 must always be used inside this function instead of CurrentPort
    void DrawCustomButtons() {
        SohImGui::GetInputEditor()->DrawControllerSelect(CurrentPort - 1);
        
        SohImGui::GetInputEditor()->DrawButton("Modificador 1", BTN_MODIFIER1, CurrentPort - 1, &BtnReading);
        SohImGui::GetInputEditor()->DrawButton("Modificador 2", BTN_MODIFIER2, CurrentPort - 1, &BtnReading);
    }

    void DrawCameraControlPanel() {
        if (!ImGui::CollapsingHeader("Controles de cámara")) {
            return;
        }

        UIWidgets::Spacer(0);
        SohImGui::BeginGroupPanel("Cámara de apuntado/primera persona", ImGui::GetContentRegionAvail());
        UIWidgets::PaddedEnhancementCheckbox("Apuntar con el stick derecho", "gRightStickAiming");
        DrawHelpIcon("Permite apuntar con el stick derecho en:\n-Vista en primera persona/C arriba\n-Apuntado de armas");
        UIWidgets::PaddedEnhancementCheckbox("Invertir eje X al apuntar", "gInvertAimingXAxis");
        DrawHelpIcon("Invierte el eje X de la cámara en:\n-Vista en primera persona/C arriba\n-Apuntado de armas");
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);
        UIWidgets::PaddedEnhancementCheckbox("Invertir eje Y al apuntar", "gInvertAimingYAxis");
        DrawHelpIcon("Invierte el eje Y de la cámara en:\n-Vista en primera persona/C arriba\n-Apuntado de armas");
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 5);
        UIWidgets::PaddedEnhancementCheckbox("Sin autocentrado en primera persona", "gDisableAutoCenterViewFirstPerson");
        DrawHelpIcon("Evita que la vista C arriba se recentre sola, permitiendo apuntar con el giroscopio");
        UIWidgets::PaddedEnhancementCheckbox("Sensibilidad personalizada al apuntar", "gEnableFirstPersonSensitivity", true, false);
        if (CVarGetInteger("gEnableFirstPersonSensitivity", 0)) {
            UIWidgets::EnhancementSliderFloat("Sensibilidad horizontal al apuntar: %d %%", "##FirstPersonSensitivity Horizontal",
                                                "gFirstPersonCameraSensitivityX", 0.01f, 5.0f, "", 1.0f, true, true);
            UIWidgets::EnhancementSliderFloat("Sensibilidad vertical al apuntar: %d %%", "##FirstPersonSensitivity Vertical",
                                              "gFirstPersonCameraSensitivityY", 0.01f, 5.0f, "", 1.0f, true, true);
        } else {
            CVarSetFloat("gFirstPersonCameraSensitivity", 1.0f);
        }
        SohImGui::EndGroupPanel();

        UIWidgets::Spacer(0);
        SohImGui::BeginGroupPanel("Cámara en tercera persona", ImGui::GetContentRegionAvail());

        UIWidgets::PaddedEnhancementCheckbox("Cámara libre", "gFreeCamera");
        DrawHelpIcon("Activa el control libre de la cámara\nNota: debes quitar los botones C del stick derecho en el "
                            "menú de configuración del mando y asignar el stick de cámara al stick derecho.");
        UIWidgets::PaddedEnhancementCheckbox("Invertir eje X de la cámara", "gInvertXAxis");
        DrawHelpIcon("Invierte el eje X de la cámara en:\n-Cámara libre");
        UIWidgets::PaddedEnhancementCheckbox("Invertir eje Y de la cámara", "gInvertYAxis");
        DrawHelpIcon("Invierte el eje Y de la cámara en:\n-Cámara libre");
+       UIWidgets::EnhancementSliderFloat("Sensibilidad horizontal (3.ª persona): %d %%", "##ThirdPersonSensitivity Horizontal",
+                                            "gThirdPersonCameraSensitivityX", 0.01f, 5.0f, "", 1.0f, true, true);
+       UIWidgets::EnhancementSliderFloat("Sensibilidad vertical (3.ª persona): %d %%", "##ThirdPersonSensitivity Vertical",
+                                          "gThirdPersonCameraSensitivityY", 0.01f, 5.0f, "", 1.0f, true, true);
        UIWidgets::EnhancementSliderInt("Distancia de cámara: %d", "##CamDist",
                                        "gFreeCameraDistMax", 100, 900, "", 185, true);
        UIWidgets::EnhancementSliderInt("Velocidad de transición: %d", "##CamTranSpeed",
                                        "gFreeCameraTransitionSpeed", 0, 900, "", 25, true);
        SohImGui::EndGroupPanel();
    }

    void DrawDpadControlPanel() {
        if (!ImGui::CollapsingHeader("Controles de la cruceta")) {
            return;
        }

        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cursor.x + 5, cursor.y + 5));
        SohImGui::BeginGroupPanel("Opciones de la cruceta", ImGui::GetContentRegionAvail());
        UIWidgets::PaddedEnhancementCheckbox("Cruceta en la pantalla de pausa", "gDpadPause");
        DrawHelpIcon("Navega por la pausa con la cruceta\nSi se usa con la cruceta para equipar objetos, debes mantener C arriba para equipar en lugar de navegar\n"
                    "Para que el cursor se mueva solo una casilla sin importar cuánto mantengas una dirección, pon gDpadHoldChange a 0 manualmente");
        UIWidgets::PaddedEnhancementCheckbox("Cruceta en cuadros de texto", "gDpadText");
        DrawHelpIcon("Navega por las opciones de los cuadros de texto, la selección de objetos en tiendas y las pantallas de selección de archivo y nombre con la cruceta\n"
                    "Para que el cursor se mueva solo una casilla al escribir el nombre sin importar cuánto mantengas una dirección, pon gDpadHoldChange a 0 manualmente");
        UIWidgets::PaddedEnhancementCheckbox("Cruceta para equipar objetos", "gDpadEquips");
        DrawHelpIcon("Equipa objetos y equipamiento con la cruceta\nSi se usa con la cruceta en la pantalla de pausa, debes mantener C arriba para equipar en lugar de navegar");
        SohImGui::EndGroupPanel();
    }

    void DrawMiscControlPanel() {
        if (!ImGui::CollapsingHeader("Controles varios")) {
            return;
        }

        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cursor.x + 5, cursor.y + 5));
        SohImGui::BeginGroupPanel("Varios", ImGui::GetContentRegionAvail());
        UIWidgets::PaddedText("Permitir el cursor en cualquier casilla");
        static const char* cursorOnAnySlot[3] = { "Solo en Rando", "Siempre", "Nunca" };
        UIWidgets::EnhancementCombobox("gPauseAnyCursor", cursorOnAnySlot, PAUSE_ANY_CURSOR_MAX, PAUSE_ANY_CURSOR_RANDO_ONLY);
        DrawHelpIcon("Permite que el cursor del menú de pausa esté sobre cualquier casilla. A veces es necesario en el rando para seleccionar "
                     "ciertos objetos.");
        UIWidgets::Spacer(0);
        UIWidgets::PaddedEnhancementCheckbox("Activar modificadores de velocidad al andar", "gEnableWalkModify", true, false);
        DrawHelpIcon("Mantén el botón asignado para cambiar la velocidad máxima al andar\nPara cambiar el botón asignado, ve a las pestañas de Puertos de arriba");
         if (CVarGetInteger("gEnableWalkModify", 0)) {
            UIWidgets::Spacer(5);
            SohImGui::BeginGroupPanel("Modificador al andar", ImGui::GetContentRegionAvail());
            UIWidgets::PaddedEnhancementCheckbox("Alternar modificador en vez de mantener", "gWalkSpeedToggle", true, false);
            UIWidgets::EnhancementSliderFloat("Modificador 1: %d %%", "##WalkMod1", "gWalkModifierOne", 0.0f, 5.0f, "", 1.0f, true);
            UIWidgets::EnhancementSliderFloat("Modificador 2: %d %%", "##WalkMod2", "gWalkModifierTwo", 0.0f, 5.0f, "", 1.0f, true);
            SohImGui::EndGroupPanel();
        }
        UIWidgets::Spacer(0);
        UIWidgets::PaddedEnhancementCheckbox("Responder a Navi con el botón L", "gNaviOnL");
        DrawHelpIcon("Habla con Navi con L pero entra en primera persona con C arriba");
        SohImGui::EndGroupPanel();

    }

    void DrawUI(bool& open) {
        if (!open) {
            CVarSetInteger("gGameControlEditorEnabled", false);
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(465, 430), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Configuración de controles del juego", &open)) {
            ImGui::BeginTabBar("##CustomControllers");
            if (ImGui::BeginTabItem("Genérico")) {
                CurrentPort = 0;
                ImGui::EndTabItem();
            }

            for (int i = 1; i <= 4; i++) {
                if (ImGui::BeginTabItem(StringHelper::Sprintf("Puerto %d", i).c_str())) {
                    CurrentPort = i;
                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();

            if (CurrentPort == 0) {
                DrawOcarinaControlPanel();
                DrawCameraControlPanel();
                DrawDpadControlPanel();
                DrawMiscControlPanel();
            } else {
                DrawCustomButtons();
            }
        }
        ImGui::End();
    }
}
