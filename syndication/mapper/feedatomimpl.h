/*
 * This file is part of the syndication library
 *
 * Copyright (C) 2006 Frank Osterfeld <osterfeld@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef SYNDICATION_MAPPER_FEEDATOMIMPL_H
#define SYNDICATION_MAPPER_FEEDATOMIMPL_H

#include <atom/document.h>
#include <feed.h>

namespace Syndication {

class FeedAtomImpl;
typedef boost::shared_ptr<FeedAtomImpl> FeedAtomImplPtr;
class Image;
typedef boost::shared_ptr<Image> ImagePtr;

/**
 * @internal
 */
class FeedAtomImpl : public Syndication::Feed
{
    public:
        
        explicit FeedAtomImpl(Syndication::Atom::FeedDocumentPtr doc);
        
        Syndication::SpecificDocumentPtr specificDocument() const;
        
        QList<Syndication::ItemPtr> items() const;
                
        QList<CategoryPtr> categories() const;
        
        QString title() const;
        
        QString link() const;
        
        QString description() const;
        
        QList<PersonPtr> authors() const;
        
        QString language() const;
        
        QString copyright() const;
        
        ImagePtr image() const;
        
        QMultiMap<QString, QDomElement> additionalProperties() const;
    private:
        
        Syndication::Atom::FeedDocumentPtr m_doc;
};

} // namespace Syndication

#endif // SYNDICATION_MAPPER_FEEDATOMIMPL_H
