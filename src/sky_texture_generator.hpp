#ifndef SKY_TEXTURE_GENERATOR_HPP
#define SKY_TEXTURE_GENERATOR_HPP

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/image.hpp>

#include "constants.hpp"
#include "sky_model.hpp"

using namespace godot;

class SkyTextureGenerator : public Node3D {
	GDCLASS(SkyTextureGenerator, Node3D)
protected:
	static void _bind_methods();

private:
	const String paramsName = "sky_parameters";

	/////////////////////////////////////////////////////////////////////////////////////
	// Helpers
	/////////////////////////////////////////////////////////////////////////////////////
	enum ParameterName {
		Albedo,
		Altitude,
		Elevation,
		Visibility,
		Resolution
	};

	template <typename T>
	String constructRange(T min, T max, T step) const {
		return String::num(min) + "," + String::num(max) + "," + String::num(step);
	}

	inline String constructParamName(ParameterName type) const {
		String name;

		switch (type) {
			case Albedo:
				name = "albedo";
				break;
			case Altitude:
				name = "altitude";
				break;
			case Elevation:
				name = "elevation";
				break;
			case Visibility:
				name = "visibility";
				break;
			case Resolution:
				name = "resolution";
				break;
			default:
				ERR_PRINT("Unknown parameter name.");
				return String();
		}

		return paramsName + String("/") + name;
	}

	/// Computes direction corresponding to given pixel coordinates in up-facing projection.
	SkyModel::Vector3 pixelToDirection(int x, int y, int resolution) const;
	/// Converts given spectrum to sRGB.
	SkyModel::Vector3 spectrumToRGB(const Spectrum &spectrum) const;
	/// Renders a simple fisheye RGB image of the sky.
	void render(SkyModel &model, std::vector<std::vector<float>> &outResult) const;

	double albedo;
	double altitude;
	double elevation;
	double visibility;
	int resolution;

	std::string outputFile;
	std::string datasetPath;
	bool datasetLoaded = false;

	SkyModel skyModel;
	SkyModel::AvailableData available;

public:
	SkyTextureGenerator();

	void setDatasetPath(String path);
	String getDatasetPath() const;

	void setAlbedo(double value);
	double getAlbedo() const;

	void setAltitude(double value);
	double getAltitude() const;

	void setElevation(double value);
	double getElevation() const;

	void setVisibility(double value);
	double getVisibility() const;

	void setResolution(int value);
	int getResolution() const;

	bool _set(const StringName &p_name, const Variant &p_property);
	bool _get(const StringName &p_name, Variant &r_property) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _property_can_revert(const StringName &p_name) const;
	bool _property_get_revert(const StringName &p_name, Variant &r_property) const;

	void readDataset();
	Ref<Image> generate() const;
};

#endif
