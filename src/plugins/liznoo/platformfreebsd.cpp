/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2013  Georg Rudoy
 * Copyright (C) 2012       Maxim Ignatenko
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

#include "platformfreebsd.h"
#include <sys/ioctl.h>
#include <dev/acpica/acpiio.h>
#include <fcntl.h>
#include <errno.h>
#include <QTimer>
#include <QMessageBox>

namespace LeechCraft
{
namespace Liznoo
{
	const int UpdateInterval = 10 * 1000;

	PlatformFreeBSD::PlatformFreeBSD (QObject *parent)
	: PlatformLayer (parent)
	{
		Timer_ = new QTimer (this);
		Timer_->start (UpdateInterval);
		connect (Timer_,
				SIGNAL (timeout ()),
				this,
				SLOT (update ()));
		ACPIfd_ = open ("/dev/acpi", O_RDONLY);

		QTimer::singleShot (0,
				this,
				SIGNAL (started ()));
	}

	void PlatformFreeBSD::Stop ()
	{
		Timer_->stop ();
	}

	void PlatformFreeBSD::ChangeState (PowerState state)
	{
		int fd = open ("/dev/acpi", O_WRONLY);
		if (fd < 0 && errno == EACCES)
		{
			QMessageBox::information (NULL,
				"LeechCraft Liznoo",
				tr ("Looks like you don't have permission to write to /dev/acpi. "
					"If you're in 'wheel' group, add 'perm acpi 0664' to "
					"/etc/devfs.conf and run '/etc/rc.d/devfs restart' to apply "
					"needed permissions to /dev/acpi."));
			return;
		}

		int sleep_state = -1;
		switch (state)
		{
		case PowerState::Suspend:
			sleep_state = 3;
			break;
		case PowerState::Hibernate:
			sleep_state = 4;
			break;
		}
		if (fd >= 0 && sleep_state > 0)
			ioctl (fd, ACPIIO_REQSLPSTATE, &sleep_state); // this requires root privileges by default
	}

	void PlatformFreeBSD::update ()
	{
		int batteries = 0;
		if (ACPIfd_ <= 0)
			return;

		ioctl (ACPIfd_, ACPIIO_BATT_GET_UNITS, &batteries);
		for (int i = 0; i < batteries; i++)
		{
			acpi_battery_ioctl_arg arg;
			BatteryInfo info;
			int units, capacity, percentage, rate, voltage, remaining_time;
			bool valid = false;
			arg.unit = i;
			if (ioctl (ACPIfd_, ACPIIO_BATT_GET_BIF, &arg) >= 0)
			{
				info.ID_ = QString("%1 %2 %3")
								.arg (arg.bif.model)
								.arg (arg.bif.serial)
								.arg (arg.bif.oeminfo);
				info.Technology_ = arg.bif.type;
				units = arg.bif.units;
				capacity = arg.bif.lfcap >= 0 ? arg.bif.lfcap : arg.bif.dcap;
			}
			arg.unit = i;
			if (ioctl (ACPIfd_, ACPIIO_BATT_GET_BATTINFO, &arg) >= 0)
			{
				percentage = arg.battinfo.cap;
				rate = arg.battinfo.rate;
				remaining_time = arg.battinfo.min * 60;
			}
			arg.unit = i;
			if (ioctl (ACPIfd_, ACPIIO_BATT_GET_BST, &arg) >= 0)
			{
				voltage = arg.bst.volt;
				if ((arg.bst.state & ACPI_BATT_STAT_INVALID) != ACPI_BATT_STAT_INVALID &&
					arg.bst.state != ACPI_BATT_STAT_NOT_PRESENT)
					valid = true;
			}

			info.Percentage_ = percentage;
			info.Voltage_ = static_cast<double> (voltage) / 1000;
			switch (units)
			{
			case ACPI_BIF_UNITS_MW:
				info.EnergyRate_ = static_cast<double> (rate) / 1000;
				info.EnergyFull_ = static_cast<double> (capacity) / 1000;
				info.Energy_ = info.EnergyFull_ * percentage / 100;
				break;
			case ACPI_BIF_UNITS_MA:
				info.EnergyRate_ = (static_cast<double> (rate) / 1000) * info.Voltage_;
				info.EnergyFull_ = (static_cast<double> (capacity) / 1000) * info.Voltage_;
				info.Energy_ = info.EnergyFull_ * percentage / 100;
				break;
			default:
				valid = false;
				break;
			}

			if (valid)
			{
				if ((arg.bst.state & ACPI_BATT_STAT_DISCHARG) != 0)
				{
					info.TimeToFull_ = 0;
					info.TimeToEmpty_ = remaining_time;
				}
				else if ((arg.bst.state & ACPI_BATT_STAT_CHARGING) != 0)
				{
					info.TimeToEmpty_ = 0;
					info.TimeToFull_ = (info.EnergyFull_ - info.Energy_) / info.EnergyRate_ * 3600;
				}
				emit batteryInfoUpdated (info);
			}
		}
	}
}
}
