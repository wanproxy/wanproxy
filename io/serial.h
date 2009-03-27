#ifndef	SERIAL_H
#define	SERIAL_H

class SerialPort : public FileDescriptor {
public:
	class Speed {
		const unsigned speed_;
	public:
		Speed(const unsigned& speed)
		: speed_(speed)
		{
			switch (speed) {
			case 115200:
				break;
			default:
				NOTREACHED();
			}
		}

		~Speed()
		{ }

		unsigned operator() (void) const
		{
			return (speed_);
		}
	};

private:
	LogHandle log_;

	SerialPort(int);
public:
	~SerialPort();

	static SerialPort *open(const std::string&, const Speed&);
};

#endif /* !SERIAL_H */
