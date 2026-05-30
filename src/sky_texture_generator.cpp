#include "sky_texture_generator.hpp"

#include <tinyexr/tinyexr.h>

#include <assert.h>
#include <array>
#include <execution>
#include <limits>
#include <string>
#include <vector>

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/string.hpp>

void SkyTextureGenerator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("generate"), &SkyTextureGenerator::generate);

	ClassDB::bind_method(D_METHOD("get_dataset_path"), &SkyTextureGenerator::getDatasetPath);
	ClassDB::bind_method(D_METHOD("set_dataset_path", "dataset.dat"), &SkyTextureGenerator::setDatasetPath);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "dataset_path", PROPERTY_HINT_GLOBAL_FILE, "*.dat"), "set_dataset_path", "get_dataset_path");
}

void SkyTextureGenerator::_ready() {
	generate();
}

String SkyTextureGenerator::getDatasetPath() const {
	return datasetPath;
}
void SkyTextureGenerator::setDatasetPath(String path) {
	datasetPath = path;
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

void SkyTextureGenerator::render(
		SkyModel &model,
		const double albedo,
		const double altitude,
		const double azimuth,
		const double elevation,
		const int resolution,
		const double visibility,
		std::vector<std::vector<float>> &outResult) const {
	assert(model.isInitialized());

	// We are viewing the sky from 'altitude' meters above the origin.
	const SkyModel::Vector3 viewPoint = SkyModel::Vector3(0.0, 0.0, altitude);

	// Resize the output buffer.
	outResult.resize(SPECTRUM_CHANNELS + 1);
	outResult[0].resize(size_t(resolution) * resolution * 3); // RGB
	for (int c = 1; c < SPECTRUM_CHANNELS + 1; c++) {
		outResult[c].resize(size_t(resolution) * resolution); // mono
	}

	// Zero the output buffer.
	for (int c = 0; c < SPECTRUM_CHANNELS + 1; c++) {
		for (int i = 0; i < outResult[c].size(); i++) {
			outResult[c][i] = 0.f;
		}
	}

	SkyModel::FrameInterpolationParameters frameIterParams = model.computeFrameInterpolationParameters(
			viewPoint,
			elevation,
			azimuth,
			visibility,
			albedo);

	std::vector<int> xs;
	xs.resize(resolution);
	for (int x = 0; x < resolution; x++) {
		xs[x] = x;
	}

	std::for_each(std::execution::par,
			xs.begin(),
			xs.end(),
			[this,
					&model,
					&frameIterParams,
					altitude,
					elevation,
					resolution,
					&viewPoint,
					visibility,
					&outResult](auto &&x) {
				for (int y = 0; y < resolution; y++) {
					// For each pixel of the rendered image get the corresponding direction in fisheye
					// projection.
					const SkyModel::Vector3 viewDir = this->pixelToDirection(x, y, resolution);

					// If the pixel lies outside the upper hemisphere, the direction will be zero. Such
					// a pixel is kept black.
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
					outResult[0][(size_t(x) * resolution + y) * 3] = float(rgb.x);
					outResult[0][(size_t(x) * resolution + y) * 3 + 1] = float(rgb.y);
					outResult[0][(size_t(x) * resolution + y) * 3 + 2] = float(rgb.z);

					// Store the individual channels.
					for (int c = 1; c < SPECTRUM_CHANNELS + 1; c++) {
						outResult[c][(size_t(x) * resolution + y)] = float(spectrum[c - 1]);
					}
				}
			});
}

void SkyTextureGenerator::generate() const {
	// Read all command-line options.
	const double albedo = 0.5;
	const double altitude = 0.0;
	const double azimuth = 0.0;
	const int channel = 0;
	const double elevation = 0.0;
	const int resolution = 64;
	const double visibility = 59.4;
	const std::string dataset = datasetPath.utf8().get_data();
	const std::string outputFile = "sky_textures/01.exr";

	std::vector<std::vector<float>> result;

	try {
		// Initialize the model with the given dataset file.
		SkyModel skyModel;
		skyModel.initialize(dataset, visibility);

		//Render sky image according to the given configuration.
		render(skyModel, albedo, altitude, azimuth, elevation, resolution, visibility, result);

		// Save the result buffer into an EXR file.
		const char *err = nullptr;
		int ret = SaveEXR(result[channel].data(), resolution, resolution, channel == 0 ? 3 : 1, 0, outputFile.c_str(), &err);
		if (ret != TINYEXR_SUCCESS) {
				const std::string message(std::string("Saving EXR failed - ") + std::string(err));
			FreeEXRErrorMessage(err); // Frees buffer for an error message
			throw std::runtime_error(message);
		}
		print_line("Done\n");
	} catch (std::exception &e) {
		ERR_PRINT(godot::String("Error: ") + e.what());
	}
}
