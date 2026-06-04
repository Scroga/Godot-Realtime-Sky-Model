@tool

class_name SkyModel
extends WorldEnvironment

## Emitted when the environment has changed to a new resource.
signal environment_changed

const SKY_SHADER: String = "res://addons/sky_generator/shaders/sky.gdshader"

## The Sun DirectionalLight.
var sun: DirectionalLight3D
## The Sky shader.
var sky_material: ShaderMaterial
# The sky texture generator
var sky_texture_generator: SkyTextureGenerator

#####################
## Texture Generation
#####################
func _read_dataset() -> void:
	if sky_texture_generator == null:
		push_error("sky_texture_generator is null")
		return

	sky_texture_generator.read_dataset()

func _generate_texture() -> void:
	if sky_texture_generator == null:
		push_error("sky_texture_generator is null")
		return

	if sky_material == null:
		push_error("sky_material is null")
		return

	var image: Image = sky_texture_generator.generate()
	
	if image == null:
		push_error("Sky image generation failed.")
		return
	
	var texture: ImageTexture = ImageTexture.create_from_image(image)
	sky_material.set_shader_parameter("skyTexture", texture)

@export_group("Texture Generation")

@export_tool_button("Read Dataset")
var read_dataset_button = _read_dataset

@export_tool_button("Generate Sky Texture")
var generate_button = _generate_texture

#####################
## Texture Parameters
#####################

@export_group("Texture Parameters")

@export_range(-10.0, 10.0, 0.1)
var exposure: float = -5.0:
	set(value):
		exposure = value
		if sky_material:
			sky_material.set_shader_parameter("exposure", exposure)


#####################
## Setup
#####################

func _notification(what: int) -> void:
	# Must be after _init and before _enter_tree to properly set vars like 'sky' for setters
	if what in [ NOTIFICATION_SCENE_INSTANTIATED, NOTIFICATION_ENTER_TREE ]:
		_initialize()

func _initialize() -> void:
	# Create default environment
	if environment == null:
		environment = Environment.new()
		environment.background_mode = Environment.BG_SKY
		environment.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
		environment.ambient_light_sky_contribution = 0.7
		environment.ambient_light_energy = 1.0
		environment.reflected_light_source = Environment.REFLECTION_SOURCE_SKY
		environment.tonemap_mode = Environment.TONE_MAPPER_ACES
		environment.tonemap_white = 6
		emit_signal("environment_changed", environment)
		
	# Setup Sky material & Upgrade old
	if environment.sky == null or environment.sky.sky_material is PhysicalSkyMaterial:
		environment.sky = Sky.new()
		environment.sky.sky_material = ShaderMaterial.new()
		environment.sky.sky_material.shader = load(SKY_SHADER)
		
	# Set a reference to the sky material for easy access.
	sky_material = environment.sky.sky_material
		
	# Create default camera attributes
	if camera_attributes == null:
		camera_attributes = CameraAttributesPractical.new()
	
	if has_node("SkyTextureGenerator"):
		sky_texture_generator = $SkyTextureGenerator
	elif is_inside_tree():
		sky_texture_generator = SkyTextureGenerator.new()
		add_child(sky_texture_generator, true)
		sky_texture_generator.owner = get_tree().edited_scene_root
		
	if has_node("SunLight"):
		sun = $SunLight
	elif is_inside_tree():
		sun = DirectionalLight3D.new()
		sun.name = "SunLight"
		add_child(sun, true)
		sun.owner = get_tree().edited_scene_root
		sun.shadow_enabled = true

func _set(property: StringName, value: Variant) -> bool:
	match property:
		"environment":
			#sky.environment = value
			environment = value
			emit_signal("environment_changed", environment)
			return true
	return false
