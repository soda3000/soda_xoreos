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
 *  Creature within an area in KotOR games.
 */

#include <cassert>

#include "external/glm/gtc/type_ptr.hpp"

#include "src/common/util.h"
#include "src/common/maths.h"
#include "src/common/error.h"
#include "src/common/ustring.h"
#include "src/common/strutil.h"
#include "src/common/debug.h"
#include "src/common/random.h"

#include "src/aurora/2dafile.h"
#include "src/aurora/2dareg.h"
#include "src/aurora/gff3file.h"
#include "src/aurora/locstring.h"
#include "src/aurora/resman.h"

#include "src/graphics/aurora/modelnode.h"
#include "src/graphics/aurora/model.h"
#include "src/graphics/aurora/animationchannel.h"

#include "src/engines/aurora/util.h"
#include "src/engines/aurora/model.h"

#include "src/engines/kotorbase/creature.h"
#include "src/engines/kotorbase/item.h"
#include "src/engines/kotorbase/creaturesearch.h"

#include "src/engines/kotorbase/gui/chargeninfo.h"

namespace Engines {

namespace KotORBase {

Creature::Creature(const Common::UString &resRef) :
		Object(kObjectTypeCreature),
		_commandable(true),
		_walkRate(0.0f),
		_runRate(0.0f) {

	init();

	std::unique_ptr<Aurora::GFF3File> utc(loadOptionalGFF3(resRef, Aurora::kFileTypeUTC));
	if (!utc)
		throw Common::Exception("Creature \"%s\" has no blueprint", resRef.c_str());

	load(utc->getTopLevel());
}

Creature::Creature(const Aurora::GFF3Struct &creature) :
		Object(kObjectTypeCreature),
		_commandable(true),
		_walkRate(0.0f),
		_runRate(0.0f) {

	init();
	load(creature);
}

Creature::Creature() :
		Object(kObjectTypeCreature),
		_commandable(true),
		_walkRate(0.0f),
		_runRate(0.0f) {

	init();
}

Creature::~Creature() {
}

void Creature::init() {
	_isPC = false;

	_appearance = Aurora::kFieldIDInvalid;

	_gender = kGenderNone;
	_race = kRaceUnknown;
	_subRace = kSubRaceNone;
	_skin = kSkinMAX;
	_face = 0;

	_headModel = 0;
	_visible = false;
}

void Creature::show() {
	if (_visible)
		return;

	_visible = true;

	if (_model)
		_model->show();
}

void Creature::hide() {
	if (!_visible)
		return;

	if (_model)
		_model->hide();

	_visible = false;
}

bool Creature::isVisible() const {
	return _visible;
}

bool Creature::isPC() const {
	return _isPC;
}

bool Creature::isPartyMember() const {
	return _isPC;
}

bool Creature::matchSearchCriteria(const Object *UNUSED(target), const CreatureSearchCriteria &UNUSED(criteria)) const {
	// TODO: Implement pattern matching
	return false;
}

bool Creature::isCommandable() const {
	return _commandable;
}

void Creature::setUsable(bool usable) {
	Object::setUsable(usable);
	if (_model)
		_model->setClickable(isClickable());
}

void Creature::setCommandable(bool commandable) {
	_commandable = commandable;
}

Gender Creature::getGender() const {
	return _gender;
}

int Creature::getLevel(const Class &c) const {
	return _info.getClassLevel(c);
}

int Creature::getLevelByPosition(int position) const {
	return _info.getLevelByPosition(position);
}

Class Creature::getClassByPosition(int position) const {
	return _info.getClassByPosition(position);
}

Race Creature::getRace() const {
	return _race;
}

SubRace Creature::getSubRace() const {
	return _subRace;
}

float Creature::getWalkRate() const {
	return _walkRate;
}

float Creature::getRunRate() const {
	return _runRate;
}

int Creature::getSkillRank(Skill skill) {
	return _info.getSkillRank(skill);
}

int Creature::getAbilityScore(Ability ability) {
	return _info.getAbilityScore(ability);
}

void Creature::setPosition(float x, float y, float z) {
	Object::setPosition(x, y, z);
	Object::getPosition(x, y, z);

	if (_model)
		_model->setPosition(x, y, z);
}

void Creature::setOrientation(float x, float y, float z, float angle) {
	Object::setOrientation(x, y, z, angle);
	Object::getOrientation(x, y, z, angle);

	if (_model)
		_model->setOrientation(x, y, z, angle);
}

void Creature::load(const Aurora::GFF3Struct &creature) {
	_templateResRef = creature.getString("TemplateResRef");

	std::unique_ptr<Aurora::GFF3File> utc;
	if (!_templateResRef.empty())
		utc.reset(loadOptionalGFF3(_templateResRef, Aurora::kFileTypeUTC, MKTAG('U', 'T', 'C', ' ')));

	load(creature, utc ? &utc->getTopLevel() : 0);

	if (!utc)
		warning("Creature \"%s\" has no blueprint", _tag.c_str());
}

void Creature::load(const Aurora::GFF3Struct &instance, const Aurora::GFF3Struct *blueprint) {
	_info = CreatureInfo(instance);

	// General properties

	if (blueprint)
		loadProperties(*blueprint);  // Blueprint
	loadProperties(instance, false); // Instance


	// Appearance

	if (_appearance == Aurora::kFieldIDInvalid)
		throw Common::Exception("Creature without an appearance");

	loadEquippedModel();

	// Position

	setPosition(instance.getDouble("XPosition"),
	            instance.getDouble("YPosition"),
	            instance.getDouble("ZPosition"));

	// Orientation

	float bearingX = instance.getDouble("XOrientation");
	float bearingY = instance.getDouble("YOrientation");

	setOrientation(0.0f, 0.0f, 1.0f, -Common::rad2deg(atan2(bearingX, bearingY)));
}

void Creature::loadProperties(const Aurora::GFF3Struct &gff, bool clearScripts) {
	// Tag
	_tag = gff.getString("Tag", _tag);

	// Name

	Aurora::LocString firstName;
	gff.getLocString("FirstName", firstName);
	Aurora::LocString lastName;
	gff.getLocString("LastName", lastName);

	if (!firstName.empty()) {
		_name = firstName.getString();
		if (!lastName.empty())
			_name += " " + lastName.getString();
	}


	// Description
	_description = gff.getString("Description", _description);

	// Portrait
	loadPortrait(gff);

	// Equipment
	loadEquipment(gff);

	// Abilities
	loadAbilities(gff);

	// Appearance
	_appearance = gff.getUint("Appearance_Type", _appearance);

	// Static
	_static = gff.getBool("Static", _static);

	// Usable
	_usable = gff.getBool("Useable", _usable);

	// PC
	_isPC = gff.getBool("IsPC", _isPC);

	// Gender
	_gender = Gender(gff.getUint("Gender"));

	// Race
	_race = Race(gff.getSint("Race", _race));
	_subRace = SubRace(gff.getSint("SubraceIndex", _subRace));

	// Hit Points
	_currentHitPoints = gff.getSint("CurrentHitPoints", _maxHitPoints);
	_maxHitPoints = gff.getSint("MaxHitPoints", _currentHitPoints);

	_minOneHitPoint = gff.getBool("Min1HP", _minOneHitPoint);

	// Faction
	_faction = Faction(gff.getUint("FactionID", _faction));

	// Scripts
	readScripts(gff, clearScripts);

	_conversation = gff.getString("Conversation", _conversation);
}

void Creature::loadPortrait(const Aurora::GFF3Struct &gff) {
	uint32_t portraitID = gff.getUint("PortraitId");
	if (portraitID != 0) {
		const Aurora::TwoDAFile &twoda = TwoDAReg.get2DA("portraits");

		Common::UString portrait = twoda.getRow(portraitID).getString("BaseResRef");
		if (!portrait.empty()) {
			if (portrait.beginsWith("po_"))
				_portrait = portrait;
			else
				_portrait = "po_" + portrait;
		}
	}

	_portrait = gff.getString("Portrait", _portrait);
}

void Creature::loadEquipment(const Aurora::GFF3Struct &gff) {
	if (!gff.hasField("Equip_ItemList"))
		return;

	for (const auto &i : gff.getList("Equip_ItemList")) {
		InventorySlot slot = InventorySlot(static_cast<int>(std::log2f(i->getID())));
		Common::UString tag = i->getString("EquippedRes");
		equipItem(tag, slot, false);
	}
}

void Creature::loadAbilities(const Aurora::GFF3Struct &gff) {
	if (gff.hasField("Str"))
		_info.setAbilityScore(kAbilityStrength, gff.getUint("Str"));
	if (gff.hasField("Dex"))
		_info.setAbilityScore(kAbilityDexterity, gff.getUint("Dex"));
	if (gff.hasField("Con"))
		_info.setAbilityScore(kAbilityConstitution, gff.getUint("Con"));
	if (gff.hasField("Int"))
		_info.setAbilityScore(kAbilityIntelligence, gff.getUint("Int"));
	if (gff.hasField("Wis"))
		_info.setAbilityScore(kAbilityWisdom, gff.getUint("Wis"));
	if (gff.hasField("Cha"))
		_info.setAbilityScore(kAbilityCharisma, gff.getUint("Cha"));
}

void Creature::getModelState(uint32_t &state, uint8_t &textureVariation) {
	state = 'a';
	textureVariation = 1;

	if (_info.isInventorySlotEquipped(kInventorySlotBody)) {
		Item *item = _equipment[kInventorySlotBody].get();
		state += item->getBodyVariation() - 1;
		textureVariation = item->getTextureVariation();
	}
}

void Creature::getPartModels(PartModels &parts, uint32_t state, uint8_t textureVariation) {
	const Aurora::TwoDARow &appearance = TwoDAReg.get2DA("appearance").getRow(_appearance);

	_modelType = appearance.getString("modeltype");

	if (_modelType == "B") {
		parts.body = appearance.getString(Common::UString("model") + state);
		parts.bodyTexture = appearance.getString(Common::UString("tex") + state) + Common::String::format("%02u", textureVariation);

		// Fall back to a default texture variation
		if (!ResMan.hasResource(parts.bodyTexture))
			parts.bodyTexture = appearance.getString(Common::UString("tex") + state) + "01";

	} else {
		parts.body = appearance.getString("race");
		parts.bodyTexture = appearance.getString("racetex");
	}

	if ((_modelType == "B") || (_modelType == "P")) {
		const int headNormalID = appearance.getInt("normalhead");
		const int headBackupID = appearance.getInt("backuphead");

		const Aurora::TwoDAFile &heads = TwoDAReg.get2DA("heads");

		if      (headNormalID >= 0)
			parts.head = heads.getRow(headNormalID).getString("head");
		else if (headBackupID >= 0)
			parts.head = heads.getRow(headBackupID).getString("head");
	}

	loadMovementRate(appearance.getString("moverate"));
}

void Creature::loadBody(PartModels &parts) {
	// Model "P_BastilaBB" has broken animations. Replace it with the
	// correct one.
	if (Common::String::equalsIgnoreCase(parts.body.c_str(), "P_BastilaBB"))
		parts.body = "P_BastilaBB02";

	GfxMan.lockFrame();
	_model.reset(loadModelObject(parts.body, parts.bodyTexture));
	GfxMan.unlockFrame();

	if (!_model)
		return;

	_ids.clear();
	_ids.push_back(_model->getID());

	_model->setTag(_tag);
	_model->setClickable(isClickable());

	if (_modelType != "B" && _modelType != "P")
		_model->addAnimationChannel(Graphics::Aurora::kAnimationChannelHead);
}

void Creature::loadHead(PartModels &parts) {
	if (!_model || parts.head.empty())
		return;

	_headModel = loadModelObject(parts.head);
	if (!_headModel)
		return;

	GfxMan.lockFrame();
	_model->attachModel("headhook", _headModel);
	GfxMan.unlockFrame();
}

void Creature::loadMovementRate(const Common::UString &name) {
	const Aurora::TwoDARow &speed = TwoDAReg.get2DA("creaturespeed").getRow("2daname", name);

	_walkRate = speed.getFloat("walkrate");
	_runRate = speed.getFloat("runrate");
}

void Creature::loadEquippedModel() {
	uint32_t state;
	uint8_t textureVariation;
	getModelState(state, textureVariation);

	PartModels parts;
	if (_isPC) {
		getPartModelsPC(parts, state, textureVariation);
		_portrait = parts.portrait;
	} else {
		getPartModels(parts, state, textureVariation);
		if ((_modelType == "P") || parts.body.empty()) {
			warning("TODO: Model \"%s\": ModelType \"%s\" (\"%s\")",
			        _tag.c_str(), _modelType.c_str(), parts.body.c_str());

			return;
		}
	}

	loadBody(parts);
	loadHead(parts);

	if (!_model)
		return;

	attachWeaponModel(kInventorySlotLeftWeapon);
	attachWeaponModel(kInventorySlotRightWeapon);

	setDefaultAnimations();

	if (_visible) {
		float x, y, z;
		getPosition(x, y, z);
		_model->setPosition(x, y, z);

		float angle;
		getOrientation(x, y, z, angle);
		_model->setOrientation(x, y, z, angle);

		_model->show();
	}
}

void Creature::attachWeaponModel(InventorySlot slot) {
	assert((slot == kInventorySlotLeftWeapon) || (slot == kInventorySlotRightWeapon));

	Common::UString hookNode;

	switch (slot) {
		case kInventorySlotLeftWeapon:
			hookNode = "lhand";
			break;
		case kInventorySlotRightWeapon:
			hookNode = "rhand";
			break;
		default:
			throw Common::Exception("Unsupported equip slot");
	}

	if (!_model->hasNode(hookNode)) {
		warning("Creature::attachWeaponModel(): Model \"%s\" does not have node \"%s\"",
		        _model->getName().c_str(), hookNode.c_str());

		return;
	}

	Graphics::Aurora::Model *weaponModel = 0;

	if (_info.isInventorySlotEquipped(slot)) {
		Item *item = _equipment[slot].get();
		weaponModel = loadModelObject(item->getModelName());
	}

	GfxMan.lockFrame();
	_model->attachModel(hookNode, weaponModel);
	GfxMan.unlockFrame();
}

void Creature::initAsFakePC() {
	_name = "Fakoo McFakeston";
	_tag  = Common::String::format("[PC: %s]", _name.c_str());

	_isPC = true;
}

void Creature::initAsPC(const CharacterGenerationInfo &chargenInfo, const CreatureInfo &info) {
	_name = chargenInfo.getName();
	_usable = false;
	_isPC = true;

	_race = kRaceHuman;
	_subRace = kSubRaceNone;

	_gender = chargenInfo.getGender();

	switch (_gender) {
		case kGenderMale:
		case kGenderFemale:
			break;
		default:
			throw Common::Exception("Unknown gender");
	}

	_info = info;

	_skin = chargenInfo.getSkin();
	_face = chargenInfo.getFace();

	_minOneHitPoint = true;
	_currentHitPoints = _maxHitPoints = 1;

	reloadEquipment();
	loadEquippedModel();
}

const Common::UString &Creature::getCursor() const {
	static Common::UString talkCursor("talk");
	static Common::UString killCursor("kill");
	static Common::UString useCursor("use");

	if (!isEnemy())
		return talkCursor;

	return isDead() ? useCursor : killCursor;
}

void Creature::highlight(bool enabled) {
	_model->drawBound(enabled);
}

bool Creature::click(Object *triggerer) {
	// Try the onDialog script first
	if (hasScript(kScriptDialogue))
		return runScript(kScriptDialogue, this, triggerer);

	// Next, look we have a generic onClick script
	if (hasScript(kScriptClick))
		return runScript(kScriptClick, this, triggerer);

	return false;
}

const Common::UString &Creature::getConversation() const {
	return _conversation;
}

CreatureInfo &Creature::getCreatureInfo() {
	return _info;
}

float Creature::getCameraHeight() const {
	float height = 1.8f;
	if (_model) {
		Graphics::Aurora::ModelNode *node = _model->getNode("camerahook");
		if (node) {
			float x, y, z;
			node->getPosition(x, y, z);
			height = z;
		}
	}
	return height;
}

void Creature::equipItem(Common::UString tag, InventorySlot slot, bool updateModel) {
	equipItem(tag, slot, _info, updateModel);
}

void Creature::equipItem(Common::UString tag, InventorySlot slot, CreatureInfo &invOwner, bool updateModel) {
	if (_info.isInventorySlotEquipped(slot)) {
		Common::UString equippedItem = _info.getEquippedItem(slot);
		_info.unequipInventorySlot(slot);
		invOwner.addInventoryItem(equippedItem);
		_equipment.erase(slot);
	}

	if (!tag.empty() && addItemToEquipment(tag, slot)) {
		invOwner.removeInventoryItem(tag);
		_info.equipItem(tag, slot);
	}

	if (!updateModel)
		return;

	switch (slot) {
		case kInventorySlotBody:
			loadEquippedModel();
			break;
		case kInventorySlotLeftWeapon:
		case kInventorySlotRightWeapon:
			attachWeaponModel(slot);
			break;
		default:
			break;
	}
}

Inventory &Creature::getInventory() {
	return _info.getInventory();
}

Item *Creature::getEquipedItem(InventorySlot slot) const {
	if (!_info.isInventorySlotEquipped(slot))
		return nullptr;

	return _equipment.find(slot)->second.get();
}

float Creature::getMaxAttackRange() const {
	const Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon);
	if (rightWeapon && rightWeapon->isRangedWeapon())
		return rightWeapon->getMaxAttackRange();

	return 1.0f;
}

void Creature::playDefaultAnimation() {
	if (_model)
		_model->playDefaultAnimation();
}

void Creature::playDefaultHeadAnimation() {
	if (!_model)
		return;

	Graphics::Aurora::AnimationChannel *headChannel = 0;

	if (_modelType == "B" || _modelType == "P") {
		Graphics::Aurora::Model *head = _model->getAttachedModel("headhook");
		if (head)
			headChannel = head->getAnimationChannel(Graphics::Aurora::kAnimationChannelAll);
	} else
		headChannel = _model->getAnimationChannel(Graphics::Aurora::kAnimationChannelHead);

	if (headChannel)
		headChannel->playDefaultAnimation();
}

void Creature::playDrawWeaponAnimation() {
	if (!_model)
		return;

	const Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon);
	const Item *leftWeapon = getEquipedItem(kInventorySlotLeftWeapon);

	if (rightWeapon && !leftWeapon) {
		switch (rightWeapon->getWeaponWield()) {
			case kWeaponWieldBaton:
				_model->playAnimation("g1w1");
				break;
			case kWeaponWieldSword:
				_model->playAnimation("g2w1");
				break;
			case kWeaponWieldStaff:
				_model->playAnimation("g3w1");
				break;
			case kWeaponWieldPistol:
				_model->playAnimation("g5w1");
				break;
			case kWeaponWieldRifle:
				_model->playAnimation("g7w1");
				break;
			default:
				break;
		}
		return;
	}

	if (rightWeapon && leftWeapon) {
		switch (rightWeapon->getWeaponWield()) {
			case kWeaponWieldSword:
				_model->playAnimation("g4w1");
				break;
			case kWeaponWieldPistol:
				_model->playAnimation("g6w1");
				break;
			default:
				break;
		}
	}
}

void Creature::playAttackAnimation() {
	if (!_model)
		return;

	const Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon);
	const Item *leftWeapon = getEquipedItem(kInventorySlotLeftWeapon);

	if (rightWeapon && !leftWeapon) {
		switch (rightWeapon->getWeaponWield()) {
			case kWeaponWieldBaton: // Baton weapon
				_model->playAnimation("g1a1"); // Default attack animation
				// TODO - Add sound for baton attack
				//playSound("");
				break;
			case kWeaponWieldSword: // Single melee
				_model->playAnimation("g2a1"); // Default attack animation
				//playSound("");
				break;
			case kWeaponWieldStaff: // Two handed melee
				_model->playAnimation("g3a1"); // Default attack animation
				//playSound("");
				break;
			case kWeaponWieldPistol: // Single pistol
				_model->playAnimation("b5a1"); // Default attack animation
				//playSound("");
				break;
			case kWeaponWieldRifle: // Two handed blaster
				_model->playAnimation("b7a1"); // Default attack animation
				//playSound("");
				break;
			default:
				break;
		}
		return;
	}

	if (rightWeapon && leftWeapon) {
		switch (rightWeapon->getWeaponWield()) {
			case kWeaponWieldSword: // Dual melee weapons
				_model->playAnimation("g4a1"); // Default attack animation
				//playSound("");
				break;
			case kWeaponWieldPistol: // Dual pistols
				_model->playAnimation("b6a1"); // Default attack animation
				//playSound("");
				break;
			default:
				break;
		}
		return;
	}

	_model->playAnimation("g8a1"); // Default attack animation
}

void Creature::playDodgeAnimation() {
	if (!_model)
		return;

	int number = getWeaponAnimationNumber();
	if (number != -1)
		_model->playAnimation(Common::String::format("g%dg1", number));
}

void Creature::playAnimation(const Common::UString &anim, bool restart, float length, float speed) {
	if (_model)
		_model->playAnimation(anim, restart, length, speed);
}

void Creature::getTooltipAnchor(float &x, float &y, float &z) const {
	if (!_model) {
		Object::getTooltipAnchor(x, y, z);
		return;
	}

	_model->getTooltipAnchor(x, y, z);
}

void Creature::updatePerception(Creature &object) {
	const float kPerceptionRange = 16.0f;

	float distance = glm::distance(
		glm::make_vec3(_position),
		glm::make_vec3(object._position));

	if (distance <= kPerceptionRange) {
		handleObjectSeen(object);
		handleObjectHeard(object);

		object.handleObjectSeen(*this);
		object.handleObjectHeard(*this);

	} else {
		handleObjectVanished(object);
		handleObjectInaudible(object);

		object.handleObjectVanished(*this);
		object.handleObjectInaudible(*this);
	}
}

bool Creature::isInCombat() const {
	return _inCombat;
}

Object *Creature::getAttackTarget() const {
	return _attackTarget;
}

int Creature::getAttackRound() const {
	return _attackRound;
}

// This method returns the object that the creature is attempting to attack.
Object *Creature::getAttemptedAttackTarget() const {
	return _attemptedAttackTarget;
}

// This method sets the object that the creature is attempting to attack.
void Creature::setAttemptedAttackTarget(Object *target) {
	_attemptedAttackTarget = target;
}

// This method starts a combat round with a specified target and round number.
void Creature::startCombat(Object *target, int round) {
	warning("DEBUG: combat started.");
	_inCombat = true; // The creature is now in combat.
	_attackTarget = target; // The target of the creature's attack is set.
	_attackRound = round; // The round number is set.
	refreshDefaultAnimations();
}

// Refresh the default animations of a creature. Used for entering and exiting combat and changing the idle animations.
void Creature::refreshDefaultAnimations() {
	//warning("refreshDefaultAnimations triggered.");
	_model->clearDefaultAnimations();
	setDefaultAnimations();
	_model->playDefaultAnimation();
	//warning("refreshDefaultAnimations completed.");
}

// This method cancels the combat by setting the creature out of combat.
void Creature::cancelCombat() {
	//warning("combat ended.");
	_inCombat = false; // The creature is no longer in combat.
	refreshDefaultAnimations();
}

// This method executes an attack on a target object.
void Creature::executeAttack(Object *target) {
	const Item *leftWeapon = getEquipedItem(kInventorySlotLeftWeapon); // Get the left weapon equipped by the creature.
	const Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon); // Get the right weapon equipped by the creature.
	int damage; // Variable to store the calculated damage.

	// Calculate the damage based on the weapons equipped.
	if (rightWeapon && leftWeapon) // If both weapons are equipped.
		damage = computeWeaponDamage(leftWeapon) + computeWeaponDamage(rightWeapon); // Damage is the sum of both weapons' damage.
	else if (rightWeapon) // If only the right weapon is equipped.
		damage = computeWeaponDamage(rightWeapon); // Damage is the right weapon's damage.
	else // If no weapons are equipped.
		damage =  1 + _info.getAbilityModifier(kAbilityStrength); // Damage is based on the creature's strength ability modifier.

	// Calculate the new hit points of the target after the attack.
	int hp = target->getCurrentHitPoints() - damage; // Subtract the damage from the target's current hit points.
	int minHp = target->getMinOneHitPoints() ?  1 :  0; // Get the minimum hit points for the target.


	// Check if the target's hit points have dropped to or below the minimum.
	if (hp <= minHp) {
		hp = minHp; // Ensure the target's hit points do not go below the minimum.
		cancelCombat(); // Cancel the combat since the target is dead.
	}

	// Set the target's new hit points.
	target->setCurrentHitPoints(hp);

	// Log the attack event.
	debugC(Common::kDebugEngineLogic,  1,
	       "Object \"%s\" was hit by \"%s\", has %d/%d HP",
	       target->getTag().c_str(), _tag.c_str(), hp, target->getMaxHitPoints());
}

// This method checks if the creature is dead.
bool Creature::isDead() const {
	return _dead; // Return true if the creature is dead, false otherwise.
}

// This method handles the death of the creature.
bool Creature::handleDeath() {
	if (!_dead && _currentHitPoints <=  0) { // If the creature is not already dead and its hit points are zero or less.
		_dead = true; // The creature is now dead.
		_model->clearDefaultAnimations(); // Clear any default animations on the creature's model.
		_model->addDefaultAnimation("dead",  100); // Add a "dead" animation to the creature's model.
		_model->playAnimation("die", false); // Play the "die" animation on the creature's model.
		setUsable(false); // This makes it so you cannot hover or click on the enemy, aka the red circle doesn't show up.
		return true; // Return true to indicate that the death was handled.
	}
	return false; // Return false if the creature is not dead.
}

// This method handles the event when the creature sees an object.
void Creature::handleObjectSeen(Object &object) {
	bool inserted = _seenObjects.insert(&object).second; // Try to insert the object into the set of seen objects.
	if (inserted) // If the object was successfully inserted.
		debugC(Common::kDebugEngineLogic,  2,
			"Creature \"%s\" have seen \"%s\"",
			_tag.c_str(), object.getTag().c_str()); // Log the event.
}

// This method handles the event when the creature no longer sees an object.
void Creature::handleObjectVanished(Object &object) {
	size_t countErased = _seenObjects.erase(&object); // Try to remove the object from the set of seen objects.
	if (countErased !=  0) // If the object was successfully removed.
		debugC(Common::kDebugEngineLogic,  2,
			"Object \"%s\" have vanished from \"%s\"",
			object.getTag().c_str(), _tag.c_str()); // Log the event.
}

// This method handles the event when the creature hears an object.
void Creature::handleObjectHeard(Object &object) {
	_heardObjects.insert(&object); // Insert the object into the set of heard objects.
}

// This method handles the event when the creature no longer hears an object.
void Creature::handleObjectInaudible(Object &object) {
	_heardObjects.erase(&object); // Remove the object from the set of heard objects.
}

void Creature::playHeadAnimation(const Common::UString &anim, bool restart, float length, float speed) {
	if (!_model)
		return;

	Graphics::Aurora::AnimationChannel *headChannel = 0;

	if (_modelType == "B" || _modelType == "P") {
		Graphics::Aurora::Model *head = _model->getAttachedModel("headhook");
		if (head)
			headChannel = head->getAnimationChannel(Graphics::Aurora::kAnimationChannelAll);
	} else
		headChannel = _model->getAnimationChannel(Graphics::Aurora::kAnimationChannelHead);

	if (headChannel)
		headChannel->playAnimation(anim, restart, length, speed);
}

const Action *Creature::getCurrentAction() const {
	return _actions.getCurrent();
}

void Creature::clearActions() {
	_actions.clear();
}

void Creature::addAction(const Action &action) {
	_actions.add(action);
}

void Creature::popAction() {
	_actions.pop();
}

void Creature::setDefaultAnimations() {
	if (!_model)
		return;

	// Default animations should be combat animations if in combat.
	if (isInCombat()) {
		warning("DEBUG: isInCombat = TRUE.");
		int number = getWeaponAnimationNumber();
		if (number != -1) {
			warning("DEBUG: switch case began.");
			switch (number)
			{
			case 1: // Baton
				warning("DEBUG: number = 1");
				_model->addDefaultAnimation("g1r1", 100);
				break;
			case 2: // One Melee
				warning("DEBUG: number = 2");
				_model->addDefaultAnimation("g2r1", 100);
				break;
			case 3: // Double-Bladed Melee
				warning("DEBUG: number = 3");
				_model->addDefaultAnimation("g3r1", 100);
				break;	
			case 4: // Two Melees
				warning("DEBUG: number = 4");
				_model->addDefaultAnimation("g4r1", 100);
				break;
			case 5: // One Pistol
				warning("DEBUG: number = 5");
				_model->addDefaultAnimation("g5r1", 100);
				break;
			case 6: // Two Pistols
				warning("DEBUG: number = 6");
				_model->addDefaultAnimation("g6r1", 100);
				break;
			case 7: // Large Blaster
				warning("DEBUG: number = 7");
				_model->addDefaultAnimation("g7r1", 100);
				break;
			case 8: // Everything else.
				warning("DEBUG: number = 8");
				_model->addDefaultAnimation("g8r1", 100);
				break;
			default:
				warning("DEBUG: default case.");
				break;
			}
		}
	}
	else {
		//warning("DEBUG: isInCombat = FALSE.");
		if (_modelType == "S" || _modelType == "L") {
			_model->addDefaultAnimation("cpause1", 100);
			//warning("DEBUG: cpause1 set.");
		}
		else {
			_model->addDefaultAnimation("pause1", 100);
			//warning("DEBUG: cpause1 set.");
		}
	}
}

void Creature::reloadEquipment() {
	for (int i = static_cast<int>(kInventorySlotHead); i < static_cast<int>(kInventorySlotMAX); ++i) {
		InventorySlot slot = InventorySlot(i);
		if (_info.isInventorySlotEquipped(slot))
			addItemToEquipment(_info.getEquippedItem(slot), slot);
	}
}

bool Creature::addItemToEquipment(const Common::UString &tag, InventorySlot slot) {
	try {
		_equipment.insert(std::make_pair(slot, std::make_unique<Item>(tag)));
	} catch (Common::Exception &e) {
		e.add("Failed to load item \"%s\"", tag.c_str());
		Common::printException(e, "WARNING: ");
		return false;
	}

	return true;
}

int Creature::getWeaponAnimationNumber() const {
	const Item *rightWeapon = getEquipedItem(kInventorySlotRightWeapon);
	const Item *leftWeapon = getEquipedItem(kInventorySlotLeftWeapon);

	if (rightWeapon && !leftWeapon) {
		switch (rightWeapon->getWeaponWield()) {
			case kWeaponWieldBaton:
				return 1;
			case kWeaponWieldSword:
				return 2;
			case kWeaponWieldStaff:
				return 3;
			case kWeaponWieldPistol:
				return 5;
			case kWeaponWieldRifle:
				return 7;
			default:
				return -1;
		}
	}

	if (rightWeapon && leftWeapon) {
		switch (rightWeapon->getWeaponWield()) {
			case kWeaponWieldSword:
				return 4;
			case kWeaponWieldPistol:
				return 6;
			default:
				return -1;
		}
	}

	return 8;
}

int Creature::computeWeaponDamage(const Item *weapon) const {
	int result = 0;
	Ability ability = weapon->isRangedWeapon() ? kAbilityDexterity : kAbilityStrength;
	int mod = _info.getAbilityModifier(ability);

	for (int i = 0; i < weapon->getNumDice(); ++i) {
		result += RNG.getNext(1, weapon->getDieToRoll() + 1);
	}

	return result + mod;
}

} // End of namespace KotORBase

} // End of namespace Engines
