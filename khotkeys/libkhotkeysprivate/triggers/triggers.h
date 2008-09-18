/*
   Copyright (C) 1999-2001 Lubos Lunak <l.lunak@kde.org>
   Copyright (C) 2008 Michael Jansen <kde@michael-jansen.biz>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _TRIGGERS_H_
#define _TRIGGERS_H_

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QUuid>

#include <KDE/KShortcut>

#include <kdemacros.h>
#include "khotkeysglobal.h"
#include "voicesignature.h"

#include "input.h"
#include "windows.h"


class KConfig;
class QKeySequence;

namespace KHotKeys
{

class Windowdef_list;
class ActionData;

class KDE_EXPORT Trigger
    {
    Q_DISABLE_COPY( Trigger )

    public:

        enum TriggerType
            {
            GestureTriggerType  = 0x01, //!< @see GestureTrigger
            ShortcutTriggerType = 0x02, //!< @see ShortcutTrigger
            WindowTriggerType   = 0x04, //!< @see WindowTrigger
            TriggerListType     = 0x08, //!< @see Trigger_list
            AllTypes            = 0xFF  //!< All types. For convenience.
            };

        Q_DECLARE_FLAGS(TriggerTypes, TriggerType)

        Trigger( ActionData* data_P );
        Trigger( KConfigGroup& cfg_P, ActionData* data_P );
        virtual ~Trigger();
        virtual void cfg_write( KConfigGroup& cfg_P ) const = 0;
        virtual Trigger* copy( ActionData* data_P ) const = 0;
        virtual const QString description() const = 0;
        static Trigger* create_cfg_read( KConfigGroup& cfg_P, ActionData* data_P );
        virtual void activate( bool activate_P ) = 0;

        /**
         * The trigger will be erased permanently
         */
        virtual void aboutToBeErased();

        /**
         * The actual type for this trigger
         */
        virtual TriggerType type() const = 0;

    protected:
        ActionData* const data;
    };

Q_DECLARE_OPERATORS_FOR_FLAGS(Trigger::TriggerTypes)


class KDE_EXPORT Trigger_list
    : public QList< Trigger* >
    {
    Q_DISABLE_COPY( Trigger_list )

    public:
        Trigger_list( const QString& comment_P ); // CHECKME nebo i data ?
        Trigger_list( KConfigGroup& cfg_P, ActionData* data_P );
        ~Trigger_list();
        void activate( bool activate_P );
        void cfg_write( KConfigGroup& cfg_P ) const;
        //! Some convenience typedef
        typedef QList< Trigger* >::Iterator Iterator;
        typedef QList< Trigger* >::ConstIterator ConstIterator;
        const QString comment() const;
        Trigger_list* copy( ActionData* data_P ) const;

        /**
         * @reimp
         */
        void aboutToBeErased();

    private:
        QString _comment;
    };

class KDE_EXPORT ShortcutTrigger
    : public QObject, public Trigger
    {
    Q_OBJECT

    typedef Trigger base;
    public:
        ShortcutTrigger( 
            ActionData* data_P,
            const KShortcut& shortcut_P,
            const QUuid &uuid = QUuid::createUuid() );

        ShortcutTrigger( 
            KConfigGroup& cfg_P,
            ActionData* data_P );

        virtual ~ShortcutTrigger();
        virtual void cfg_write( KConfigGroup& cfg_P ) const;
        virtual ShortcutTrigger* copy( ActionData* data_P ) const;
        virtual const QString description() const;
        KShortcut shortcut() const;
        virtual void activate( bool activate_P );

        void set_key_sequence( const QKeySequence &seq );

        virtual TriggerType type() const { return ShortcutTriggerType; }

        /**
         * @reimp
         */
        void aboutToBeErased();

    Q_SIGNALS:

        //! Emitted when the global shortcut is changed from somewhere else
        //  (Global Shortcuts KCM)
        void globalShortcutChanged(const QKeySequence&);

    public Q_SLOTS:

        void trigger();

    private:

        //! A persistent identifier for this shortcut
        QUuid _uuid;
    };

class KDE_EXPORT WindowTrigger
    : public QObject, public Trigger
    {
    Q_OBJECT
    typedef Trigger base;
    public:
        enum window_action_t
            {
            WINDOW_APPEARS         = ( 1 << 0 ),
            WINDOW_DISAPPEARS      = ( 1 << 1 ),
            WINDOW_ACTIVATES       = ( 1 << 2 ),
            WINDOW_DEACTIVATES     = ( 1 << 3 )
            };
        WindowTrigger( ActionData* data_P, Windowdef_list* windows_P, int window_actions_P );
        WindowTrigger( KConfigGroup& cfg_P, ActionData* data_P );
        virtual ~WindowTrigger();
        virtual void cfg_write( KConfigGroup& cfg_P ) const;
        virtual WindowTrigger* copy( ActionData* data_P ) const;
        virtual const QString description() const;
        const Windowdef_list* windows() const;
        bool triggers_on( window_action_t w_action_P ) const;
        virtual void activate( bool activate_P );

        virtual TriggerType type() const { return WindowTriggerType; }
    protected: // CHECKME neco private ?
        Windowdef_list* _windows;
        int window_actions;
        void init();
        typedef QMap< WId, bool > Windows_map;
        Windows_map existing_windows;
        WId last_active_window;
    protected Q_SLOTS:
        void window_added( WId window_P );
        void window_removed( WId window_P );
        void active_window_changed( WId window_P );
        void window_changed( WId window_P, unsigned int dirty_P );
    protected:
        bool active;
    };

class KDE_EXPORT GestureTrigger
    : public QObject, public Trigger
    {
    Q_OBJECT
    typedef Trigger base;
    public:
        GestureTrigger( ActionData* data_P, const QString& gesture_P );
        GestureTrigger( KConfigGroup& cfg_P, ActionData* data_P );
        virtual ~GestureTrigger();
        virtual void cfg_write( KConfigGroup& cfg_P ) const;
        virtual Trigger* copy( ActionData* data_P ) const;
        virtual const QString description() const;
        const QString& gesturecode() const;
        virtual void activate( bool activate_P );

        virtual TriggerType type() const { return GestureTriggerType; }
    protected Q_SLOTS:
        void handle_gesture( const QString& gesture_P, WId window_P );
    private:
        QString _gesturecode;
    };


// FIXME: SOUND
#if 0
class KDE_EXPORT Voice_trigger
    : public QObject, public Trigger
    {
    Q_OBJECT
    typedef Trigger base;
    public:
        Voice_trigger( ActionData* data_P, const QString& Voice_P, const VoiceSignature & signature1_P, const VoiceSignature & signature2_P );
        Voice_trigger( KConfigGroup& cfg_P, ActionData* data_P );
        virtual ~Voice_trigger();
        virtual void cfg_write( KConfigGroup& cfg_P ) const;
        virtual Trigger* copy( ActionData* data_P ) const;
        virtual const QString description() const;
        const QString& voicecode() const;
        virtual void activate( bool activate_P );
        VoiceSignature voicesignature( int ech ) const;
        
        virtual TriggerType type() const { return SoundTrigger; }
    public slots:
        void handle_Voice(  );
    private:
        QString _voicecode;
        VoiceSignature _voicesignature[2];
    };
#endif

// FIXME: SOUND
#if 0
// Voice_trigger
inline
const QString& Voice_trigger::voicecode() const
    {
    return _voicecode;
    }

inline
VoiceSignature Voice_trigger::voicesignature(int ech) const
    {
    return _voicesignature[ech-1];
    }
#endif 
} // namespace KHotKeys

#endif
