/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  The global shader material manager.
 */

#include "src/common/util.h"

#include "src/graphics/shader/materialman.h"

DECLARE_SINGLETON(Graphics::Shader::MaterialManager)

namespace Graphics {

namespace Shader {

MaterialManager::MaterialManager() {
}

// Destructor for the MaterialManager class
MaterialManager::~MaterialManager() {
    // Call the deinit method to clean up resources
    deinit();
}

// Method to initialize the MaterialManager
void MaterialManager::init() {
	// Log a status message indicating the start of the initialization process
	status("Initialising default materials...");

	// Create a new ShaderMaterial object with a shader from the ShaderMan and a name "defaultWhite"
	ShaderMaterial *material = new ShaderMaterial(ShaderMan.getShaderObject("default/colour.frag", SHADER_FRAGMENT), "defaultWhite");
	// Get the color data from the material (assuming it's a float array)
	float *color = (float *)(material->getVariableData("_colour"));
	// If the color data is valid, set it to white
	if (color) {
		color[0] = 1.0f; // Red
		color[1] = 1.0f; // Green
		color[2] = 1.0f; // Blue
		color[3] = 1.0f; // Alpha
	}
	// Add the material to the resource map with its name as the key
	_resourceMap[material->getName()] = material;

	// Repeat the process for a black material
	material = new ShaderMaterial(ShaderMan.getShaderObject("default/colour.frag", SHADER_FRAGMENT), "defaultBlack");
	color = (float *)(material->getVariableData("_colour"));
	if (color) {
		color[0] = 0.0f; // Red
		color[1] = 0.0f; // Green
		color[2] = 0.0f; // Blue
		color[3] = 1.0f; // Alpha
	}
	_resourceMap[material->getName()] = material;

	// Repeat the process for a 25% transparent black material
	material = new ShaderMaterial(ShaderMan.getShaderObject("default/colour.frag", SHADER_FRAGMENT), "defaultBlack75");
	color = (float *)(material->getVariableData("_colour"));
	if (color) {
		color[0] = 0.0f; // Red
		color[1] = 0.0f; // Green
		color[2] = 0.0f; // Blue
		color[3] = 0.75f; // Alpha (25% transparent)
	}
	_resourceMap[material->getName()] = material;

	// Repeat the process for a 50% transparent black material
	material = new ShaderMaterial(ShaderMan.getShaderObject("default/colour.frag", SHADER_FRAGMENT), "defaultBlack50");
	color = (float *)(material->getVariableData("_colour"));
	if (color) {
		color[0] = 0.0f; // Red
		color[1] = 0.0f; // Green
		color[2] = 0.0f; // Blue
		color[3] = 0.5f; // Alpha (50% transparent)
	}
	_resourceMap[material->getName()] = material;

	// Repeat the process for a grey material
	material = new ShaderMaterial(ShaderMan.getShaderObject("default/colour.frag", SHADER_FRAGMENT), "defaultGrey50");
	color = (float *)(material->getVariableData("_colour"));
	if (color) {
		color[0] = 0.5f; // Red
		color[1] = 0.5f; // Green
		color[2] = 0.5f; // Blue
		color[3] = 0.5f; // Alpha
	}
	_resourceMap[material->getName()] = material;

}

// Method to dinitialize the MaterialManager, which cleans up all the ShaderMaterial objects
void MaterialManager::deinit() {
	// Iterate over the resource map, which maps names to ShaderMaterial pointers
	for (std::map<Common::UString, ShaderMaterial *>::iterator iter = _resourceMap.begin(); iter != _resourceMap.end(); ++iter) {
		// Delete each ShaderMaterial object
		delete iter->second;
	}
	// Clear the resource map to remove all entries
	_resourceMap.clear();
}

// Method to clean up unused ShaderMaterial objects from the MaterialManager
void MaterialManager::cleanup() {
	// Iterate over the resource map, which maps names to ShaderMaterial pointers
	std::map<Common::UString, ShaderMaterial *>::iterator iter = _resourceMap.begin();
	while (iter != _resourceMap.end()) {
		// Get the current ShaderMaterial object from the iterator
		ShaderMaterial *material = iter->second;
		// Check if the use count of the material is zero, indicating it is not in use
		if (material->useCount() ==  0) {
			// If the material is not in use, delete it and remove it from the resource map
			iter = delResource(iter);
		} else {
			// If the material is still in use, move to the next entry in the resource map
			iter++;
		}
	}
}

// Method to add a new ShaderMaterial to the MaterialManager's resource map
void MaterialManager::addMaterial(ShaderMaterial *material) {
	// Check if the provided material pointer is not null
	if (!material) {
		// If the material is null, there is nothing to add, so exit the function
		return;
	}

	// Attempt to find an existing material with the same name in the resource map
	std::map<Common::UString, ShaderMaterial *>::iterator iter = _resourceMap.find(material->getName());
	// Check if the material with the given name does not already exist in the resource map
	if (iter == _resourceMap.end()) {
		// If the material is not found, add it to the resource map using its name as the key
		_resourceMap[material->getName()] = material;
	}
}

// Method to remove a ShaderMaterial from the MaterialManager's resource map
void MaterialManager::delMaterial(ShaderMaterial *material) {
	// Guard clause to return early if the provided material pointer is null
	if (!material) {
		return;
	}

	// Attempt to locate the material in the resource map by its name
	std::map<Common::UString, ShaderMaterial *>::iterator iter = _resourceMap.find(material->getName());
	// Check if the material was found in the resource map
	if (iter != _resourceMap.end()) {
		// If found, remove the material from the resource map and clean up the resource
		delResource(iter);
	}
}

// Method to retrieve a ShaderMaterial from the MaterialManager's resource map by name
ShaderMaterial *MaterialManager::getMaterial(const Common::UString &name) {
	// Attempt to find the material in the resource map by its name
	std::map<Common::UString, ShaderMaterial *>::iterator iter = _resourceMap.find(name);
	// Check if the material was found in the resource map
	if (iter != _resourceMap.end()) {
		// If found, return the ShaderMaterial pointer
		return iter->second;
	} else {
		// If not found, return a null pointer
		return nullptr;
	}
}

// Method to delete a ShaderMaterial from the MaterialManager's resource map and clean up the resource
std::map<Common::UString, ShaderMaterial *>::iterator MaterialManager::delResource(std::map<Common::UString, ShaderMaterial *>::iterator iter) {
	// Get the next iterator before deleting the current material
	std::map<Common::UString, ShaderMaterial *>::iterator inext = iter;
	inext++;
	// Delete the ShaderMaterial object
	delete iter->second;
	// Remove the material from the resource map
	_resourceMap.erase(iter);

	// Return the next iterator to maintain the integrity of the map iteration
	return inext;
}

} // End of namespace Shader

} // End of namespace Graphics
