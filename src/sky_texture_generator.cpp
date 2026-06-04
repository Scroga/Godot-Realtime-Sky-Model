#include "sky_texture_generator.hpp"

#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <assert.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <execution>
#include <limits>
#include <string>
#include <vector>

SkyTextureGenerator::SkyTextureGenerator() {
	available.albedoMin = 0.0;
	available.albedoMax = 1.0;
	available.altitudeMin = 0.0;
	available.altitudeMax = 15000.0;
	available.elevationMin = -4.2;
	available.elevationMax = 90.0;
	available.visibilityMin = 20.0;
	available.visibilityMax = 131.8;
	available.polarisation = true;
	available.channels = SPECTRUM_CHANNELS;
	available.channelStart = SPECTRUM_WAVELENGTHS[0] - 0.5 * SPECTRUM_STEP;
	available.channelWidth = SPECTRUM_STEP;

	albedo = 0.5;
	altitude = 0.0;
	elevation = 0.0;
	visibility = 59.4;
	resolution = 128;
	

	outputFile = "sky.exr";
	datasetPath = "SkyModelDataset.dat";
}

void SkyTextureGenerator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("generate"), &SkyTextureGenerator::generate);
	ClassDB::bind_method(D_METHOD("read_dataset"), &SkyTextureGenerator::readDataset);

	ClassDB::bind_method(D_METHOD("set_dataset_path", "path"), &SkyTextureGenerator::setDatasetPath);
	ClassDB::bind_method(D_METHOD("get_dataset_path"), &SkyTextureGenerator::getDatasetPath);

	ClassDB::bind_method(D_METHOD("set_albedo", "value"), &SkyTextureGenerator::setAlbedo);
	ClassDB::bind_method(D_METHOD("get_albedo"), &SkyTextureGenerator::getAlbedo);

	ClassDB::bind_method(D_METHOD("set_altitude", "value"), &SkyTextureGenerator::setAltitude);
	ClassDB::bind_method(D_METHOD("get_altitude"), &SkyTextureGenerator::getAltitude);

	ClassDB::bind_method(D_METHOD("set_elevation", "value"), &SkyTextureGenerator::setElevation);
	ClassDB::bind_method(D_METHOD("get_elevation"), &SkyTextureGenerator::getElevation);

	ClassDB::bind_method(D_METHOD("set_visibility", "value"), &SkyTextureGenerator::setVisibility);
	ClassDB::bind_method(D_METHOD("get_visibility"), &SkyTextureGenerator::getVisibility);

	ClassDB::bind_method(D_METHOD("set_resolution", "value"), &SkyTextureGenerator::setResolution);
	ClassDB::bind_method(D_METHOD("get_resolution"), &SkyTextureGenerator::getResolution);
}

bool SkyTextureGenerator::_set(const StringName &p_name, const Variant &p_value) {
	String name = String(p_name);

	if (name == "dataset_path") {
		setDatasetPath(String(p_value));
		return true;
	}

	if (name == constructParamName(Albedo)) {
		setAlbedo(double(p_value));
		return true;
	}

	if (name == constructParamName(Altitude)) {
		setAltitude(double(p_value));
		return true;
	}

	if (name == constructParamName(Elevation)) {
		setElevation(double(p_value));
		return true;
	}

	if (name == constructParamName(Visibility)) {
		setVisibility(double(p_value));
		return true;
	}

	if (name == constructParamName(Resolution)) {
		setResolution(int(p_value));
		return true;
	}

	return false;
}

bool SkyTextureGenerator::_get(const StringName &p_name, Variant &r_ret) const {
	String name = String(p_name);

	if (name == "dataset_path") {
		r_ret = String(datasetPath.c_str());
		return true;
	}

	if (name == constructParamName(Albedo)) {
		r_ret = albedo;
		return true;
	}

	if (name == constructParamName(Altitude)) {
		r_ret = altitude;
		return true;
	}

	if (name == constructParamName(Elevation)) {
		r_ret = elevation;
		return true;
	}

	if (name == constructParamName(Visibility)) {
		r_ret = visibility;
		return true;
	}

	if (name == constructParamName(Resolution)) {
		r_ret = resolution;
		return true;
	}

	return false;
}

void SkyTextureGenerator::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(
			Variant::STRING,
			"dataset_path",
			PROPERTY_HINT_GLOBAL_FILE,
			"*.dat"));

	if (!datasetLoaded) {
		return;
	}

	p_list->push_back(PropertyInfo(
			Variant::FLOAT,
			constructParamName(Albedo),
			PROPERTY_HINT_RANGE,
			constructRange(available.albedoMin, available.albedoMax, 0.001)));

	p_list->push_back(PropertyInfo(
			Variant::FLOAT,
			constructParamName(Altitude),
			PROPERTY_HINT_RANGE,
			constructRange(available.altitudeMin, available.altitudeMax, 1.0)));

	p_list->push_back(PropertyInfo(
			Variant::FLOAT,
			constructParamName(Elevation),
			PROPERTY_HINT_RANGE,
			constructRange(available.elevationMin, available.elevationMax, 0.1) + ",degrees"));

	p_list->push_back(PropertyInfo(
			Variant::FLOAT,
			constructParamName(Visibility),
			PROPERTY_HINT_RANGE,
			constructRange(available.visibilityMin, available.visibilityMax, 0.1)));

	p_list->push_back(PropertyInfo(
			Variant::INT,
			constructParamName(Resolution),
			PROPERTY_HINT_RANGE,
			constructRange(32, 4096, 1)));
}

bool SkyTextureGenerator::_property_can_revert(const StringName &p_name) const {
	String name = String(p_name);

	return name == constructParamName(Albedo) ||
			name == constructParamName(Altitude) ||
			name == constructParamName(Elevation) ||
			name == constructParamName(Visibility) ||
			name == constructParamName(Resolution);
}
bool SkyTextureGenerator::_property_get_revert(const StringName &p_name, Variant &r_property) const {
	String name = String(p_name);

	if (name == constructParamName(Albedo)) {
		r_property = 0.5;
		return true;
	}

	if (name == constructParamName(Altitude)) {
		r_property = 0.0;
		return true;
	}

	if (name == constructParamName(Elevation)) {
		r_property = 0.0;
		return true;
	}

	if (name == constructParamName(Visibility)) {
		r_property = 59.4;
		return true;
	}

	if (name == constructParamName(Resolution)) {
		r_property = 128;
		return true;
	}

	return false;
}

void SkyTextureGenerator::setDatasetPath(String path) {
	datasetPath = path.utf8().get_data();
}
String SkyTextureGenerator::getDatasetPath() const {
	return godot::String(datasetPath.c_str());
}

void SkyTextureGenerator::setAlbedo(double value) {
	albedo = std::clamp(value, available.albedoMin, available.albedoMax);
}
double SkyTextureGenerator::getAlbedo() const {
	return albedo;
}

void SkyTextureGenerator::setAltitude(double value) {
	altitude = std::clamp(value, available.altitudeMin, available.altitudeMax);
}
double SkyTextureGenerator::getAltitude() const {
	return altitude;
}

void SkyTextureGenerator::setElevation(double value) {
	elevation = std::clamp(value, available.elevationMin, available.elevationMax);
}
double SkyTextureGenerator::getElevation() const {
	return elevation;
}

void SkyTextureGenerator::setVisibility(double value) {
	visibility = std::clamp(value, available.visibilityMin, available.visibilityMax);
}
double SkyTextureGenerator::getVisibility() const {
	return visibility;
}

void SkyTextureGenerator::setResolution(int value) {
	resolution = std::clamp(value, 32, 4096);
}
int SkyTextureGenerator::getResolution() const {
	return resolution;
}

SkyModel::Vector3 SkyTextureGenerator::pixelToDirection(int x, int y, int resolution) const {
	// Make circular image area in center of image.
	const double radius = double(resolution) / 2;
	const double scaledx = (x + 0.5 - radius) / radius;
	const double scaledy = (y + 0.5 - radius) / radius;
	const double denom = scaledx * scaledx + scaledy * scaledy + 1;

	if (denom > 2.0) {
		// Outside image area.
		return SkyModel::Vector3();
	} else {
		return SkyModel::Vector3(2.0 * scaledx / denom, 2.0 * scaledy / denom, -(denom - 2.0) / denom);
	}
}

SkyModel::Vector3 SkyTextureGenerator::spectrumToRGB(const Spectrum &spectrum) const {
	// Spectrum to XYZ
	SkyModel::Vector3 xyz = SkyModel::Vector3();
	for (int wl = 0; wl < SPECTRUM_CHANNELS; wl++) {
		const int responseIdx = int((SPECTRUM_WAVELENGTHS[wl] - SPECTRAL_RESPONSE_START) / SPECTRAL_RESPONSE_STEP);
		if (0 <= responseIdx && responseIdx < std::size(SPECTRAL_RESPONSE)) {
			xyz = xyz + SPECTRAL_RESPONSE[responseIdx] * spectrum[wl];
		}
	}
	xyz = xyz * SPECTRUM_STEP;

	// XYZ to sRGB
	SkyModel::Vector3 rgb = SkyModel::Vector3();
	rgb.x = 3.2404542 * xyz.x - 1.5371385 * xyz.y - 0.4985314 * xyz.z;
	rgb.y = -0.9692660 * xyz.x + 1.8760108 * xyz.y + 0.0415560 * xyz.z;
	rgb.z = 0.0556434 * xyz.x - 0.2040259 * xyz.y + 1.0572252 * xyz.z;

	return rgb;
}

SkyModel::Vector3 SkyTextureGenerator::rotateAroundZ(const SkyModel::Vector3 &v, double angle) const {
	const double c = std::cos(angle);
	const double s = std::sin(angle);
	return SkyModel::Vector3(c * v.x - s * v.y, s * v.x + c * v.y, v.z);
}

unsigned char SkyTextureGenerator::pixToTex(const float pixel) const {
	return (unsigned char)(std::floor(std::clamp(255.f, 0.f, 255.f)));
}

void SkyTextureGenerator::render(SkyModel &model, std::vector<float> &outResult) const {
	assert(model.isInitialized());
	const unsigned int xTextureSize = resolution / 2;
	const unsigned int yTextureSize = resolution;

	// Resize the output buffer.
	outResult.resize(size_t(xTextureSize) * yTextureSize * 3);

	// We are viewing the sky from 'altitude' meters above the origin.
	const SkyModel::Vector3 viewPoint = SkyModel::Vector3(0.0, 0.0, altitude);

	const double azimuth = 0.0;
	SkyModel::FrameInterpolationParameters frameIterParams = model.computeFrameInterpolationParameters(
			viewPoint,
			degreesToRadians(elevation),
			degreesToRadians(azimuth),
			visibility,
			albedo);

	std::vector<int> xs;
	xs.resize(xTextureSize);
	for (int x = 0; x < xTextureSize; x++) {
		xs[x] = x;
	}

	std::for_each(std::execution::par, xs.begin(), xs.end(),
			[this, &model, &frameIterParams, &viewPoint, &xTextureSize , &yTextureSize, &outResult](auto &&x) {
				for (int y = 0; y < yTextureSize; y++) {
					// For each pixel of the rendered image get the corresponding direction in fisheye projection.
					const int sourceX = x + xTextureSize;
					SkyModel::Vector3 viewDir = this->pixelToDirection(sourceX, y, resolution);
					viewDir = this->rotateAroundZ(viewDir, degreesToRadians(90.0));

					// If the pixel lies outside the upper hemisphere, the direction will be zero. Such a pixel is kept black.
					if (viewDir.isZero()) {
						continue;
					}

					SkyModel::PixelInterpolationParameters pixelIterParams = model.computePixelInterpolationParameters(viewDir);

					Spectrum spectrum;
					for (int wl = 0; wl < SPECTRUM_CHANNELS; wl++) {
						spectrum[wl] = model.skyRadiance(pixelIterParams, frameIterParams, SPECTRUM_WAVELENGTHS[wl]);
					}

					// Convert the spectral quantity to sRGB and store it at 0 in the result buffer.
					const SkyModel::Vector3 rgb = this->spectrumToRGB(spectrum);

					const size_t index = (size_t(y) * xTextureSize + x) * 3;

					outResult[index + 0] = float(rgb.x);
					outResult[index + 1] = float(rgb.y);
					outResult[index + 2] = float(rgb.z);
				}
			});
}

void SkyTextureGenerator::readDataset() {
	try {
		skyModel.initialize(datasetPath, visibility);
		available = skyModel.getAvailableData();

		albedo = std::clamp(albedo, available.albedoMin, available.albedoMax);
		altitude = std::clamp(altitude, available.altitudeMin, available.altitudeMax);
		elevation = std::clamp(elevation, available.elevationMin, available.elevationMax);
		visibility = std::clamp(visibility, available.visibilityMin, available.visibilityMax);

		datasetLoaded = true;

		notify_property_list_changed();
		print_line("Dataset loaded.");
	} catch (const std::exception &e) {
		datasetLoaded = false;

		notify_property_list_changed();

		ERR_PRINT(String("Failed to load dataset: ") + String(e.what()));
	}
}

Ref<Image> SkyTextureGenerator::generate() const {
	const unsigned int xTextureSize = resolution / 2;
	const unsigned int yTextureSize = resolution;

	std::vector<float> result;
	Ref<Image> image;

	try {
		// Initialize the model with the given dataset file.
		SkyModel skyModel;
		skyModel.initialize(datasetPath, visibility);

		//Render sky image according to the given configuration.
		render(skyModel, result);

		if (result.empty()) {
			ERR_PRINT("Render result is empty.");
			return Ref<Image>();
		}

		// Save the result buffer into an godot::Image.
		PackedByteArray bytes;
		bytes.resize(xTextureSize * yTextureSize * 3 * sizeof(float));
		memcpy(bytes.ptrw(), result.data(), bytes.size());

		image.instantiate();

		image->set_data(xTextureSize, yTextureSize, false, Image::FORMAT_RGBF, bytes);

		print_line("Done\n");
	} catch (std::exception &e) {
		ERR_PRINT(String("Error: ") + e.what());
		return Ref<Image>();
	}
	return image;
}
