@tool
extends EditorPlugin

const TextureGenerator: Script = preload("res://addons/sky_generator/src/texture_generator_tool.gd")
const TextureGeneratorIcon: Texture2D = preload("res://addons/sky_generator/icons/01_icon.png")

func _enable_plugin() -> void:
	# Add autoloads here.
	pass


func _disable_plugin() -> void:
	# Remove autoloads here.
	pass

func _enter_tree() -> void:
	add_custom_type("SkyTextureGeneratorTool", "SkyTextureGenerator", TextureGenerator, TextureGeneratorIcon)

func _exit_tree() -> void:
	remove_custom_type("SkyTextureGeneratorTool")
