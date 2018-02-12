#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <vector>
#include <cstddef>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <array>
#include <stack>

constexpr bool debug = false;

void loadROM(const std::string& fname, std::vector<uint8_t>& memory)
{
	std::ifstream rom{fname, std::ios::binary | std::ios::ate | std::ios::in};

	if (!rom)
		throw std::runtime_error("ROM not found");

	size_t size = rom.tellg();
	std::cout << "size = " << size << " / " << 0xFFF - 0x200 << '\n';
	rom.seekg(0);
	rom.read(reinterpret_cast<char*>(&memory[0x200]), size);

	if (size > 0xFFF - 0x200)
		throw std::runtime_error("ROM too large");
}

constexpr std::array<uint8_t, 5 * 16> glyphs
{{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80, // F
}};

int main()
{
	std::vector<uint8_t> memory(0x1000);
	std::copy(glyphs.begin(), glyphs.end(), memory.begin());

	// ROM loading
	{
		std::string fname;
		std::cout << "File ROM path: ";
		std::cin >> fname;
		std::cout << "Loading ROM... ";
		loadROM(fname, memory);
		std::cout << "OK.\n";
	}

	// SFML windowing
	std::cout << "Initializing visuals... ";
	sf::RenderWindow win({64, 32}, "CHIP-8 emulator");
	sf::Image fb;
	fb.create(64, 32);
	std::cout << "OK.\n";

	std::array<uint8_t, 16> regs8{};
	uint8_t& regvf = regs8[0xF];
	uint16_t regi = 0, regpc = 0x200;
	uint16_t keypad;

	std::stack<uint16_t> stack;

	while (win.isOpen())
	{
		sf::Event ev;
		while (win.pollEvent(ev))
		{
			if (ev.type == sf::Event::Closed)
			{
				win.close();
			}
			else if (ev.type == sf::Event::KeyPressed)
			{
				switch (ev.key.code)
				{
				case sf::Keyboard::Num2:  keypad |= (1 << 0x1); break;
				case sf::Keyboard::Num3:  keypad |= (1 << 0x2); break;
				case sf::Keyboard::Num4:  keypad |= (1 << 0x3); break;
				case sf::Keyboard::Num5:  keypad |= (1 << 0xC); break;
				case sf::Keyboard::Z:     keypad |= (1 << 0x4); break;
				case sf::Keyboard::E:     keypad |= (1 << 0x5); break;
				case sf::Keyboard::R:     keypad |= (1 << 0x6); break;
				case sf::Keyboard::T:	  keypad |= (1 << 0xD); break;
				case sf::Keyboard::S:	  keypad |= (1 << 0x7); break;
				case sf::Keyboard::D:     keypad |= (1 << 0x8); break;
				case sf::Keyboard::F:     keypad |= (1 << 0x9); break;
				case sf::Keyboard::G:     keypad |= (1 << 0xE); break;
				case sf::Keyboard::X:     keypad |= (1 << 0xA); break;
				case sf::Keyboard::C:     keypad |= (1 << 0x0); break;
				case sf::Keyboard::V:     keypad |= (1 << 0xB); break;
				case sf::Keyboard::B:     keypad |= (1 << 0xF); break;
				default: break;
				};
			}
			else if (ev.type == sf::Event::KeyReleased)
			{
				// TODO: implement properly
				keypad = 0;
			}
			else if (ev.type == sf::Event::Resized)
			{
				win.setView(sf::View({0.f, 0.f, 64.f, 32.f}));
			}
		}

		win.clear();

		uint16_t opcode = memory[regpc] << 8 | memory[regpc + 1];
		uint8_t type = opcode >> 12;
		uint8_t &vx = regs8[(opcode >> 8) & 0x000F],
				&vy = regs8[(opcode >> 4) & 0x000F];
		uint16_t nnn = opcode & 0x0FFF, n = opcode & 0x000F;
		uint8_t kk = opcode & 0x00FF;

		if constexpr (debug)
		{
			std::cout << "Handling one instruction : " << std::hex << opcode << '\n';
		}

		regpc += 2; // 1 instruction

		if (opcode == 0x00E0) // CLS
		{
			if constexpr (debug)
			{
				std::cout << "Clearing framebuffer.\n";
			}
			fb.create(64, 32);
		}
		else if (opcode == 0x00EE) // RET
		{
			if (stack.empty())
			{
				std::cout << "Program tried to return with an empty stack.\n";
				return 1;
			}

			regpc = stack.top();
			stack.pop();
		}
		else
		{
			switch (type)
			{
			case 0x1:
				regpc = nnn;
			break;

			case 0x2:
				if (stack.size() >= 16)
				{
					std::cout << "Reached callstack limit\n";
					return 1;
				}
				stack.push(regpc);
				regpc = nnn;
			break;

			case 0x3:
				if (vx == kk)
				{
					regpc += 2;
				}
			break;

			case 0x4:
				if (vx != kk)
				{
					regpc += 2;
				}
			break;

			case 0x5:
				if (n == 0)
				{
					if (vx == vy)
					{
						regpc += 2;
					}
					else
					{
						// TODO: crash
						return 1;
					}
				}
			break;

			case 0x6:
				vx = kk;
			break;

			case 0x7:
				vx += kk;
			break;

			case 0x8:
				switch (n)
				{
				case 0x0: vx = vy; break;
				case 0x1: vx |= vy; break;
				case 0x2: vx &= vy; break;
				case 0x3: vx ^= vy; break;

				case 0x4:
					regvf = vx + vy > 255;
					vx += vy;
				break;

				case 0x5:
					regvf = vx > vy;
					vx -= vy;
				break;

				case 0x6:
					regvf = vx & 0b00000001;
					vx >>= 1;
				break;

				case 0x7:
					regvf = vx > vy;
					vx = vy - vx;
				break;

				case 0xE:
					regvf = vx & 0b10000000;
					vx <<= 1;
				break;

				default:
					// TODO: crash
					return 1;
				}
			break;

			case 0x9:
				if (n == 0x0)
				{
					if (vx != vy)
					{
						regpc += 2;
					}
				}
				else
				{
					// TODO: crash
					return 1;
				}
			break;

			case 0xA:
				regi = nnn;
			break;

			case 0xB:
				regpc = nnn + regs8[0];
			break;

			case 0xC:
				vx = (rand() % 256) & kk;
			break;

			case 0xD: {
				uint8_t x = regs8[(opcode >> 8) & 0xF], y = regs8[(opcode >> 4) & 0xF], n = opcode & 0xF;
				uint8_t *sprite = &memory[regi]; // TODO: memory check

				if constexpr (debug)
				{
					std::cout << "Drawing " << std::dec << +n << " bytes from sprite 0x" << std::hex << std::setw(4) << std::setfill('0') << regi << std::dec << " to (" << +x << ", " << +y << ")\n";
				}

				if (n > 15)
				{
					std::cout << "Program tried to draw a over 15 byte sprite\n";
					break;
				}

				regvf = 0; // Reset VF, before we perhaps set the collision flag below

				for (size_t byte = 0; byte < n; ++byte) // Display n bytes
				for (size_t i = 0; i <= 7; ++i) // Display individual bits
				{
					// TODO handle the below cases as expected: wrap-around
					if (x + i    >= 64) continue;
					if (y + byte >= 32) continue;

					bool currentPixel = fb.getPixel(x + i, y + byte).r > 0;
					bool spritePixel = sprite[byte] & (1 << (7 - i));

					if (currentPixel && spritePixel)
					{
						regvf = 1; // Set collision flag
					}

					fb.setPixel(x + i, y + byte, ((currentPixel ^ spritePixel) ? sf::Color::White : sf::Color::Black));
				}
			} break;

			case 0xE:
				switch (kk)
				{
				case 0x9E:
					if ((keypad >> vx) & 0x1)
					{
						regpc += 2;
					}
				break;

				case 0xA1:
					if (!((keypad >> vx) & 0x1))
					{
						regpc += 2;
					}
				break;

				default:
					// TODO: crash
					return 1;
				};
			break;

			case 0xF:
				switch (kk)
				{
				case 0x07:
					if constexpr (debug)
					{
						std::cout << "Unimplemented timer instruction\n";
					}
					vx = 0;
				break;

				case 0x0A:
					std::cout << "Unimplemented keypad\n";
				break;

				case 0x15:
				case 0x18:
					std::cout << "Unimplemented audio instruction\n";
				break;

				case 0x29:
					regi = 5 * (vx & 0xF);
				break;

				case 0x33:
					// HACK: temporary
					memory[regi + 0] = 0;
					memory[regi + 1] = 0;
					memory[regi + 2] = vx;
				break;

				case 0x1E:
					regi += vx;
				break;

				case 0x55:
					std::copy(&memory[regi], &memory[regi] + (&vx - &regs8[0]), &regs8[0]);
				break;

				case 0x65:
					std::copy(&regs8[0], &vx, &memory[regi]);
				break;

				default:
					// TODO: crash
					return 1;
				};
			break;

			default:
				std::cout << "Unhandled opcode at 0x" << std::hex << std::setw(4) << std::setfill('0') << regpc <<
							 ": 0x" << std::setw(4) << std::setfill('0') << opcode << ".\n";
				for (;;);
				return 1;
			};
		}

		if (regpc >= 0xFFF)
		{
			std::cout << "Program counter exceeded 0xFFF address.\n";
			return 1;
		}

		// Show texture
		sf::Texture tex;
		tex.loadFromImage(fb);
		win.draw(sf::Sprite(tex));

		win.display();
	}
}
