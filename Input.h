#pragma once

#include <unordered_map>

namespace AB
{
	class Input
	{
	public:
		
		static Input& Get()
		{
			if (!instance)
				instance = new Input();
			return *instance;
		}

		bool KeyDown(const char key);
		bool KeyPressed(const char key);
		bool KeyReleased(const char key);

		void Init();
		void Update();

		Input(Input const&) = delete;
		void operator=(Input const&) = delete;

		~Input();

		std::unordered_map<char, bool>& GetKeymap();

	private:

		static Input* instance;

		Input() {};

		std::unordered_map<char, bool> keymap;
		std::unordered_map<char, bool> prevkeymap;
	};
}