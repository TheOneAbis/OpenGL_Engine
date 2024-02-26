#pragma once

#include <unordered_map>
#include <glm/glm.hpp>

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

		bool MouseButtonDown(int button);
		bool MouseButtonPressed(int button);
		bool MouseButtonReleased(int button);

		glm::vec2& GetMousePos();
		glm::vec2& GetLastMousePos();
		glm::vec2 GetMouseDelta();

		void Init();
		void Update();

		Input(Input const&) = delete;
		void operator=(Input const&) = delete;

		~Input();

		std::unordered_map<char, bool>& GetKeyMap();
		std::unordered_map<int, bool>& GetMouseMap();
	private:

		static Input* instance;

		Input() {};

		std::unordered_map<char, bool> keymap;
		std::unordered_map<char, bool> prevkeymap;
		std::unordered_map<int, bool> mousemap;
		std::unordered_map<int, bool> prevmousemap;

		glm::vec2 mousePos;
		glm::vec2 prevMousePos;
	};
}