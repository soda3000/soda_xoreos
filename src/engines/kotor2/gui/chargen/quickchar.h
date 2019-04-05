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
 *  The KotOR 2 quick character panel.
 */

#ifndef ENGINES_KOTOR2_GUI_CHARGEN_QUICKCHAR_H
#define ENGINES_KOTOR2_GUI_CHARGEN_QUICKCHAR_H

#include "src/engines/kotor2/gui/gui.h"
#include "src/engines/kotor2/gui/chargen/charactergeneration.h"

namespace Engines {

namespace KotOR2 {

class QuickCharPanel : public KotORBase::GUI {
public:
	QuickCharPanel(CharacterGeneration* chargen, Console *console = 0);

protected:
	void initWidget(Widget &widget) override;

private:
	void updateButtons();

	void callbackActive(Widget &widget);

	CharacterGeneration *_chargenMenu;
};

} // End of namespace KotOR2

} // End of namespace Engines

#endif // ENGINES_KOTOR2_GUI_CHARGEN_QUICKCHAR_H
