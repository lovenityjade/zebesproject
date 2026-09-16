import unreal
from pathlib import Path
root = Path(unreal.Paths.project_dir()).resolve().parent
path = '/Game/SM/M_Present'
material = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
if material is None:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_Present', '/Game/SM', unreal.Material, unreal.MaterialFactoryNew())
mel = unreal.MaterialEditingLibrary
mel.delete_all_material_expressions(material)
material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_OPAQUE)
textures = {}
for i,name in enumerate(['GameTexture','SceneTexture','EnvironmentMask','EffectsTexture','GuiTexture','CinemaTexture']):
    tex = mel.create_material_expression(material, unreal.MaterialExpressionTextureObjectParameter, -1100, -150-i*120)
    tex.set_editor_property('parameter_name',name)
    tex.set_editor_property('texture',unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture'))
    textures[name]=tex
uv = mel.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -600, 150)
scalars = {'Enabled':1.0, 'Gameplay':0.0, 'Intensity':0.35, 'Brightness':1.0,
           'BlendMode':0.0, 'SceneFade':1.0, 'EngineWeather':0.0, 'ViewWidth':256.0, 'HudTop':32.0, 'CombatEffects':0.0, 'FlashStrength':1.0, 'SporeAtmosphere':0.0}

vectors = {'CinemaState':(0,0,0,0), 'CreditsState':(0,0,0,0), 'WeatherState':(0,0,0,0), 'WeatherView':(0,0,32767,0), 'ChargeState':(0,0,0,0), 'PowerBombState':(0,0,0,0), 'PowerBombPhase':(0,0,0,0), 'GrappleState':(0,0,0,0), 'GrappleEndState':(0,0,0,0), 'ScrewState':(0,0,0,0)}
parameters = {}
for i, (name, value) in enumerate(scalars.items()):
    node = mel.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -650, 220+i*100)
    node.set_editor_property('parameter_name', name)
    node.set_editor_property('default_value', value)
    parameters[name] = node
for i, (name, value) in enumerate(vectors.items()):
    node = mel.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -900, 220+i*100)
    node.set_editor_property('parameter_name', name)
    node.set_editor_property('default_value', unreal.LinearColor(*value))
    parameters[name] = node
custom = mel.create_material_expression(material, unreal.MaterialExpressionCustom, -200, 0)
custom.set_editor_property('code', (root / 'Shaders/Environment.usf').read_text() + '\n' + (root / 'Shaders/Present.usf').read_text().replace('// CINEMATIC_EFFECTS',(root / 'Shaders/Cinematics.usf').read_text()))
custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = []
for name in list(textures) + ['UV'] + list(parameters):
    item = unreal.CustomInput()
    item.set_editor_property('input_name', name)
    inputs.append(item)
custom.set_editor_property('inputs', inputs)
for name,tex in textures.items():
    mel.connect_material_expressions(tex, '', custom, name)
mel.connect_material_expressions(uv, '', custom, 'UV')
for name, node in parameters.items():
    mel.connect_material_expressions(node, 'RGBA' if name in vectors else '', custom, name)
mel.connect_material_property(custom, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(material)
unreal.EditorAssetLibrary.save_asset(path)
unreal.log('SM_MATERIAL_READY ' + path)
