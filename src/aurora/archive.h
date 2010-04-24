/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file aurora/archive.h
 *  Handling various archive files.
 */

#ifndef AURORA_ARCHIVE_H
#define AURORA_ARCHIVE_H

#include <list>

#include "common/types.h"
#include "common/ustring.h"

#include "aurora/types.h"

namespace Common {
	class SeekableReadStream;
}

namespace Aurora {

class Archive {
public:
	struct Resource {
		Common::UString name;  ///< The resource's name.
		FileType        type;  ///< The resource's type.
		uint32          index; ///< The resource's local index within the archive.
	};

	typedef std::list<Resource> ResourceList;

	Archive();
	virtual ~Archive();

	/** Clear the resource list. */
	virtual void clear() = 0;

	/** Return the list of resources. */
	virtual const ResourceList &getResources() const = 0;

	/** Return a stream of the resource's contents. */
	virtual Common::SeekableReadStream *getResource(uint32 index) const = 0;
};

} // End of namespace Aurora

#endif // AURORA_ARCHIVE_H
