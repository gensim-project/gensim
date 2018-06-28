/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/generic/ps2/PS2Device.h"
#include "util/LogContext.h"

UseLogContext(LogDevice);
DeclareChildLogContext(LogPS2, LogDevice, "PS2");
DeclareChildLogContext(LogPS2Keyboard, LogPS2, "Keyboard");
DeclareChildLogContext(LogPS2Mouse, LogPS2, "Mouse");

/* Keyboard Commands */
#define KBD_CMD_SET_LEDS		0xED	/* Set keyboard leds */
#define KBD_CMD_ECHO			0xEE
#define KBD_CMD_SCANCODE		0xF0	/* Get/set scancode set */
#define KBD_CMD_GET_ID			0xF2	/* get keyboard ID */
#define KBD_CMD_SET_RATE		0xF3	/* Set typematic rate */
#define KBD_CMD_ENABLE			0xF4	/* Enable scanning */
#define KBD_CMD_RESET_DISABLE	0xF5	/* reset and disable scanning */
#define KBD_CMD_RESET_ENABLE   	0xF6    /* reset and enable scanning */
#define KBD_CMD_RESET			0xFF	/* Reset */

/* Keyboard Replies */
#define KBD_REPLY_POR		0xAA	/* Power on reset */
#define KBD_REPLY_ID		0xAB	/* Keyboard ID */
#define KBD_REPLY_ACK		0xFA	/* Command ACK */
#define KBD_REPLY_RESEND	0xFE	/* Command NACK, send the cmd again */

/* Mouse Commands */
#define AUX_SET_SCALE11		0xE6	/* Set 1:1 scaling */
#define AUX_SET_SCALE21		0xE7	/* Set 2:1 scaling */
#define AUX_SET_RES			0xE8	/* Set resolution */
#define AUX_GET_SCALE		0xE9	/* Get scaling factor */
#define AUX_SET_STREAM		0xEA	/* Set stream mode */
#define AUX_POLL			0xEB	/* Poll */
#define AUX_RESET_WRAP		0xEC	/* Reset wrap mode */
#define AUX_SET_WRAP		0xEE	/* Set wrap mode */
#define AUX_SET_REMOTE		0xF0	/* Set remote mode */
#define AUX_GET_TYPE		0xF2	/* Get type */
#define AUX_SET_SAMPLE		0xF3	/* Set sample rate */
#define AUX_ENABLE_DEV		0xF4	/* Enable aux device */
#define AUX_DISABLE_DEV		0xF5	/* Disable aux device */
#define AUX_SET_DEFAULT		0xF6
#define AUX_RESET			0xFF	/* Reset aux device */
#define AUX_ACK				0xFA	/* Command byte ACK. */

using namespace archsim::abi::devices::generic::ps2;

static archsim::abi::devices::ComponentDescriptor ps2_descriptor ("PS2Device");

PS2Device::PS2Device(IRQLine& irq) : Component(ps2_descriptor), irq(irq), irq_enabled(false)
{

}

bool PS2Device::Initialise()
{
	return true;
}


PS2KeyboardDevice::PS2KeyboardDevice(IRQLine& irq) : PS2Device(irq), last_command(0)
{

}

PS2MouseDevice::PS2MouseDevice(IRQLine& irq) : PS2Device(irq), last_command(0), status(0), resolution(0), sample_rate(0), last_x(0), last_y(0), button_state(0)
{

}

void PS2KeyboardDevice::SendCommand(uint32_t command)
{
	LC_DEBUG1(LogPS2Keyboard) << "Sending Keyboard Command: " << std::hex << command;

	switch (last_command) {
		case 0:
			switch (command) {
				case KBD_CMD_RESET:
					QueueData(KBD_REPLY_ACK);
					QueueData(KBD_REPLY_POR);
					break;

				case KBD_CMD_GET_ID:
					QueueData(KBD_REPLY_ACK);
					QueueData(KBD_REPLY_ID);
					QueueData(0x41);
					break;

				case KBD_CMD_SCANCODE:
				case KBD_CMD_SET_LEDS:
				case KBD_CMD_SET_RATE:
					last_command = command;
					QueueData(KBD_REPLY_ACK);
					break;

				default:
					LC_DEBUG1(LogPS2Keyboard) << "Unhandled Keyboard Command - but acknowledging anyway.";
					QueueData(KBD_REPLY_ACK);
					break;
			}
			break;

		case KBD_CMD_SCANCODE:
			last_command = 0;
			if (command == 0) {
				// GET SCANCODE SET
				QueueData(0x41);
			} else {
				// SET SCANCODE SET
				QueueData(KBD_REPLY_ACK);
			}
			break;

		case KBD_CMD_SET_LEDS:
			last_command = 0;
			QueueData(KBD_REPLY_ACK);
			break;

		case KBD_CMD_SET_RATE:
			last_command = 0;
			QueueData(KBD_REPLY_ACK);
			break;

		default:
			last_command = 0;
			QueueData(KBD_REPLY_ACK);

			LC_DEBUG1(LogPS2Keyboard) << "Keyboard state out-of-phase";

			break;
	}

}

void PS2KeyboardDevice::KeyDown(uint32_t keycode)
{
	if ((keycode & 0xff00) == 0xe0)
		QueueData(0xe0);
	QueueData(keycode & 0xff);
}

void PS2KeyboardDevice::KeyUp(uint32_t keycode)
{
	if ((keycode & 0xff00) == 0xe0)
		QueueData(0xe0);
	QueueData(0xf0);
	QueueData(keycode & 0xff);
}

void PS2MouseDevice::SendCommand(uint32_t command)
{
	LC_DEBUG1(LogPS2Mouse) << "Sending Mouse Command: " << std::hex << command;

	switch (last_command) {
		case 0:
			switch (command) {
				case AUX_RESET:
					status = 0;
					resolution = 2;
					sample_rate = 100;

					QueueData(AUX_ACK);
					QueueData(0xaa);
					QueueData(0x00);
					break;

				case AUX_GET_TYPE:
					QueueData(AUX_ACK);
					QueueData(0x00);
					break;

				case AUX_SET_DEFAULT:
					status = 0;
					resolution = 2;
					sample_rate = 100;

					QueueData(AUX_ACK);
					break;

				case AUX_SET_SAMPLE:
				case AUX_SET_RES:
					last_command = command;
					QueueData(AUX_ACK);
					break;

				case AUX_GET_SCALE:
					QueueData(AUX_ACK);
					QueueData(status);
					QueueData(resolution);
					QueueData(sample_rate);
					break;

				default:
					LC_DEBUG1(LogPS2Mouse) << "Unhandled Mouse Command - but acknowledging anyway.";
					QueueData(AUX_ACK);
					break;
			}
			break;

		case AUX_SET_SAMPLE:
			sample_rate = command;
			last_command = 0;
			QueueData(AUX_ACK);
			break;

		case AUX_SET_RES:
			resolution = command;
			last_command = 0;
			QueueData(AUX_ACK);
			break;

		default:
			last_command = 0;
			QueueData(AUX_ACK);

			LC_DEBUG1(LogPS2Keyboard) << "Mouse state out-of-phase";
			break;
	}
}

void PS2MouseDevice::ButtonDown(int button_index)
{
	button_state |= 1 << button_index;

	SendUpdate(0,0);
}

void PS2MouseDevice::ButtonUp(int button_index)
{
	button_state &= ~(1 << button_index);

	SendUpdate(0,0);
}

void PS2MouseDevice::Move(int x, int y)
{
	int32_t dx = x - last_x;
	int32_t dy = y - last_y;

	last_x = x;
	last_y = y;

	SendUpdate(dx, dy);
}

void PS2MouseDevice::SendUpdate(int32_t dx, int32_t dy)
{
	if (dx > 127)
		dx = 127;
	else if (dx < -127)
		dx = -127;

	if (dy > 127)
		dy = 127;
	else if (dy < -127)
		dy = -127;

	QueueData(0x08 | ((dx < 0) << 4) | ((dy < 0) << 5) | (button_state & 0x07) );
	QueueData(dx & 0xff);
	QueueData(dy & 0xff);
}
