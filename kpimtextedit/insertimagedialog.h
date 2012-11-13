/*
  Copyright (c) 2012 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#ifndef INSERTIMAGEDIALOG_H
#define INSERTIMAGEDIALOG_H
#include <KDialog>
#include <KUrl>

namespace KPIMTextEdit {

class InsertImageDialogPrivate;
class InsertImageDialog : public KDialog
{
  Q_OBJECT
public:
  explicit InsertImageDialog(QWidget *parent = 0);
  ~InsertImageDialog();

  int imageWidth() const;
  int imageHeight() const;

  void setImageWidth(int value);
  void setImageHeight(int value);

  KUrl imageUrl() const;
  void setImageUrl(const KUrl&url);

  bool keepOriginalSize() const;

private:
  friend class InsertImageDialogPrivate;
  InsertImageDialogPrivate *d;
  Q_PRIVATE_SLOT( d, void _k_slotKeepOriginalSizeClicked(bool) )
  Q_PRIVATE_SLOT( d, void _k_slotUrlChanged(const QString&))
  Q_PRIVATE_SLOT( d, void _k_slotImageWidthChanged(int))
  Q_PRIVATE_SLOT( d, void _k_slotImageHeightChanged(int))
};
}

#endif // INSERTIMAGEDIALOG_H