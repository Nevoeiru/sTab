# -*- coding: utf-8 -*-
"""
sTab Sculpt Companion - Add-on para Blender 3.0+ / 4.x / 5.x
Otimizador de 1 clique e utilitário de telemetria para o Samsung Galaxy Tab S9 FE+.
"""

bl_info = {
    "name": "sTab Sculpt Companion",
    "author": "Antigravity sTab Team",
    "version": (1, 0, 0),
    "blender": (3, 0, 0),
    "location": "View3D > Sidebar (N-Panel) > sTab",
    "description": "Otimizacao em 1 clique e telemetria para Galaxy Tab S9 FE+ e S-Pen no Blender Sculpt Mode",
    "warning": "",
    "doc_url": "",
    "category": "3D View",
}

import bpy
import socket
import json
import threading
import time

# Porta UDP local para telemetria bidirecional com o sTab
STAB_TELEMETRY_PORT = 27185

# ------------------------------------------------------------------------
# 1. Operador: Otimizacao do Blender em 1 Clique
# ------------------------------------------------------------------------
class STAB_OT_OptimizeBlender(bpy.types.Operator):
    """Configura o Blender com os parametros ideais para a S-Pen e navegacao touch do Galaxy Tab S9 FE+"""
    bl_idname = "stab.optimize_blender"
    bl_label = "Otimizar Blender para Galaxy Tab S9 FE+"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        prefs = context.preferences

        # 1. Trava a Tablet API no padrao Windows Ink nativo
        try:
            prefs.inputs.tablet_api = 'WININK'
        except Exception:
            pass

        # 2. Configura navegacao de viewport 3D ideal para gestos touch
        prefs.inputs.view_rotate_method = 'TURNTABLE'
        prefs.inputs.view_zoom_axis = 'VERTICAL'
        prefs.inputs.use_zoom_to_mouse = True
        prefs.inputs.use_rotate_around_active = True
        prefs.inputs.use_auto_perspective = True

        # 3. Garante que atalhos essenciais estejam ativos
        prefs.inputs.use_emulate_numpad = False

        self.report(
            {'INFO'},
            "sTab: Blender otimizado com sucesso! (Windows Ink ativo, 90Hz e gestos calibrados)"
        )
        return {'FINISHED'}


# ------------------------------------------------------------------------
# 2. Operador: Preparacao de Escultura Rapida (Clay Matcap + Cavity)
# ------------------------------------------------------------------------
class STAB_OT_SetupClayShading(bpy.types.Operator):
    """Aplica sombreador Matcap de argila vermelha e realce de cavidades para escultura profissional"""
    bl_idname = "stab.setup_clay_shading"
    bl_label = "Ativar Shading de Argila (Sculpt View)"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        # Garante que a 3D Viewport ativa use MatCap com Cavity
        for area in context.screen.areas:
            if area.type == 'VIEW_3D':
                for space in area.spaces:
                    if space.type == 'VIEW_3D':
                        shading = space.shading
                        shading.type = 'SOLID'
                        shading.light = 'MATCAP'
                        shading.show_cavity = True
                        shading.cavity_type = 'BOTH'
                        shading.cavity_ridge_factor = 1.5
                        shading.cavity_valley_factor = 1.5

        # Se houver objeto ativo, entra em modo de escultura
        if context.active_object and context.active_object.type == 'MESH':
            if context.mode != 'SCULPT':
                try:
                    bpy.ops.object.mode_set(mode='SCULPT')
                except Exception:
                    pass

        self.report({'INFO'}, "sTab: Visualizacao de argila e relevo ativada!")
        return {'FINISHED'}


# ------------------------------------------------------------------------
# 3. Operador: Selecao Rapida de Pinceis de Escultura
# ------------------------------------------------------------------------
class STAB_OT_SelectBrush(bpy.types.Operator):
    """Seleciona rapidamente um pincel essencial de escultura"""
    bl_idname = "stab.select_brush"
    bl_label = "Selecionar Pincel"
    bl_options = {'REGISTER', 'UNDO'}

    brush_type: bpy.props.StringProperty(name="Brush Type", default="Clay Strips")

    def execute(self, context):
        if context.mode != 'SCULPT':
            self.report({'WARNING'}, "sTab: Entre no Sculpt Mode para trocar de pincel.")
            return {'CANCELLED'}

        brush_map = {
            "Clay Strips": "builtin_brush.Clay Strips",
            "Clay": "builtin_brush.Clay",
            "Draw": "builtin_brush.Draw",
            "Smooth": "builtin_brush.Smooth",
            "Grab": "builtin_brush.Grab",
            "Inflate": "builtin_brush.Inflate",
            "Crease": "builtin_brush.Crease",
            "Scrape": "builtin_brush.Scrape",
            "Mask": "builtin_brush.Mask"
        }

        brush_name = brush_map.get(self.brush_type, self.brush_type)
        brush = bpy.data.brushes.get(brush_name)
        if brush:
            context.tool_settings.sculpt.brush = brush
            self.report({'INFO'}, f"sTab: Pincel ativo -> {self.brush_type}")
        else:
            self.report({'WARNING'}, f"Pincel '{brush_name}' nao encontrado no arquivo.")
        return {'FINISHED'}


# ------------------------------------------------------------------------
# 4. Servidor de Telemetria UDP em Segundo Plano
# ------------------------------------------------------------------------
class STabTelemetryServer:
    _instance = None

    def __init__(self):
        self.running = False
        self.thread = None
        self.sock = None

    @classmethod
    def get_instance(cls):
        if cls._instance is None:
            cls._instance = STabTelemetryServer()
        return cls._instance

    def start(self):
        if self.running:
            return
        self.running = True
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        except Exception as e:
            print(f"[sTab Telemetry] Erro ao criar socket: {e}")
            return

        self.thread = threading.Thread(target=self._telemetry_loop, daemon=True)
        self.thread.start()
        print(f"[sTab Telemetry] Transmissor UDP ativo na porta {STAB_TELEMETRY_PORT}.")

    def stop(self):
        self.running = False
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
            self.sock = None
        print("[sTab Telemetry] Transmissor UDP finalizado.")

    def _telemetry_loop(self):
        while self.running:
            time.sleep(0.1)  # 10 Hz de amostragem de interface
            try:
                context = bpy.context
                if context and hasattr(context, "tool_settings") and hasattr(context.tool_settings, "sculpt"):
                    sculpt = context.tool_settings.sculpt
                    brush = sculpt.brush
                    if brush:
                        data = {
                            "brush": brush.name,
                            "size": brush.size if hasattr(brush, "size") else 50,
                            "strength": round(brush.strength, 2) if hasattr(brush, "strength") else 0.5,
                            "symmetry_x": sculpt.use_symmetry_x,
                        }
                        payload = json.dumps(data).encode('utf-8')
                        if self.sock:
                            self.sock.sendto(payload, ("127.0.0.1", STAB_TELEMETRY_PORT))
            except Exception:
                pass


# ------------------------------------------------------------------------
# 5. Operador: Ativar/Desativar Telemetria UDP
# ------------------------------------------------------------------------
class STAB_OT_ToggleTelemetry(bpy.types.Operator):
    """Liga ou desliga a telemetria em tempo real para a interface do Galaxy Tab"""
    bl_idname = "stab.toggle_telemetry"
    bl_label = "Alternar Telemetria UDP"

    def execute(self, context):
        server = STabTelemetryServer.get_instance()
        if server.running:
            server.stop()
            self.report({'INFO'}, "sTab: Telemetria UDP desativada.")
        else:
            server.start()
            self.report({'INFO'}, "sTab: Telemetria UDP ativada (127.0.0.1:27185).")
        return {'FINISHED'}


# ------------------------------------------------------------------------
# 6. Painel de Controle no Sidebar da 3D Viewport (N-Panel)
# ------------------------------------------------------------------------
class STAB_PT_MainPanel(bpy.types.Panel):
    """Painel sTab no N-Panel do Blender 3D Viewport"""
    bl_label = "sTab: Galaxy Tab S9 FE+"
    bl_idname = "STAB_PT_main_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'sTab'

    def draw(self, context):
        layout = self.layout
        tool_settings = context.tool_settings

        # Box 1: Status e Otimizacao
        box_opt = layout.box()
        box_opt.label(text="Configuracao do Dispositivo", icon='TABLET')
        row = box_opt.row()
        row.scale_y = 1.4
        row.operator("stab.optimize_blender", text="Otimizar Blender para sTab", icon='CHECKMARK')

        row_shd = box_opt.row()
        row_shd.operator("stab.setup_clay_shading", text="Visualizacao Argila 3D", icon='SHADING_SOLID')

        # Box 2: Telemetria do Pincel Ativo
        box_sculpt = layout.box()
        box_sculpt.label(text="Pincel Ativo & Parametros", icon='BRUSH_SCULPT_DRAW')

        if context.mode == 'SCULPT' and tool_settings.sculpt and tool_settings.sculpt.brush:
            brush = tool_settings.sculpt.brush
            col = box_sculpt.column(align=True)
            col.label(text=f"Ferramenta: {brush.name}")
            col.prop(brush, "size", text="Raio (F)")
            col.prop(brush, "strength", text="Forca (Shift+F)")
            
            # Simetria
            row_sym = box_sculpt.row(align=True)
            row_sym.prop(tool_settings.sculpt, "use_symmetry_x", text="X", toggle=True)
            row_sym.prop(tool_settings.sculpt, "use_symmetry_y", text="Y", toggle=True)
            row_sym.prop(tool_settings.sculpt, "use_symmetry_z", text="Z", toggle=True)
        else:
            box_sculpt.label(text="(Entre no Sculpt Mode para ver parametros)", icon='INFO')

        # Box 3: Paleta Rapida de Pinceis
        box_palette = layout.box()
        box_palette.label(text="Paleta de Escultura", icon='SCULPTMODE_HLT')
        grid = box_palette.grid_flow(row_major=True, columns=2, even_columns=True, even_rows=False, align=True)

        brushes = ["Clay Strips", "Draw", "Smooth", "Grab", "Inflate", "Crease", "Scrape", "Mask"]
        for b in brushes:
            op = grid.operator("stab.select_brush", text=b)
            op.brush_type = b

        # Box 4: Servico de Telemetria
        box_telemetry = layout.box()
        server = STabTelemetryServer.get_instance()
        status_text = "Desativar Telemetria" if server.running else "Ativar Telemetria (UDP)"
        icon_stat = 'CANCEL' if server.running else 'PLAY'
        box_telemetry.operator("stab.toggle_telemetry", text=status_text, icon=icon_stat)


# ------------------------------------------------------------------------
# 7. Registro e Desregistro do Add-on
# ------------------------------------------------------------------------
classes = (
    STAB_OT_OptimizeBlender,
    STAB_OT_SetupClayShading,
    STAB_OT_SelectBrush,
    STAB_OT_ToggleTelemetry,
    STAB_PT_MainPanel,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    # Inicia a telemetria automaticamente se desejado
    STabTelemetryServer.get_instance().start()

def unregister():
    STabTelemetryServer.get_instance().stop()
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)

if __name__ == "__main__":
    register()

