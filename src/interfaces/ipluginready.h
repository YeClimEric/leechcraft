/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#ifndef INTERFACES_IPLUGINREADY_H
#define INTERFACES_IPLUGINREADY_H
#include <QtPlugin>
#include <QSet>

/** @brief Base class for plugins accepting second-level plugins.
 *
 * A plugin for LeechCraft could be actually a plugin for another
 * plugin. Then, to simplify the process, if a plugin could handle such
 * second-level plugins (if it's a host for them), it's better to
 * implement this interface. LeechCraft would the automatically manage
 * the dependencies, perform correct initialization order and feed the
 * matching first-level plugins with second-level ones.
 *
 * Plugins of different levels are matched with each other by their
 * classes, which is returned by IPlugin2::GetPluginClasses() and by
 * IPluginReady::GetExpectedPluginClasses().
 */
class Q_DECL_EXPORT IPluginReady
{
public:
	virtual ~IPluginReady () {}

	/** @brief Returns the expected classes of the plugins for this
	 * plugin.
	 *
	 * Returns the expected second level plugins' classes expected by this
	 * first-level plugin.
	 *
	 * @note This function should be able to work before IInfo::Init() is
	 * called.
	 *
	 * @return The expected plugin class entity.
	 */
	virtual QSet<QByteArray> GetExpectedPluginClasses () const = 0;

	/** @brief Adds second-level plugin to this one.
	 *
	 * @note This function should be able to work before IInfo::Init() is
	 * called. The second-level plugin also comes uninitialized.
	 *
	 * @param[in] plugin The pointer to the plugin instance.
	 */
	virtual void AddPlugin (QObject *plugin) = 0;
};

Q_DECLARE_INTERFACE (IPluginReady, "org.Deviant.LeechCraft.IPluginReady/1.0");

#endif

