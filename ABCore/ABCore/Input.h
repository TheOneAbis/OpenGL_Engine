#pragma once

#include <unordered_map>

typedef struct GLFWwindow GLFWwindow;

namespace glm
{
	enum precision;
	namespace detail
	{
		template <typename T, precision P>
		struct tvec2;
	};
	typedef detail::tvec2<float, (precision)0> vec2;
}

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

		bool KeyDown(int key);
		bool KeyPressed(int key);
		bool KeyReleased(int key);

		bool MouseButtonDown(int button);
		bool MouseButtonPressed(int button);
		bool MouseButtonReleased(int button);

		glm::vec2& GetMousePos();
		glm::vec2& GetLastMousePos();
		glm::vec2 GetMouseDelta();

		void Init(GLFWwindow* window);
		void Update();

		Input(Input const&) = delete;
		void operator=(Input const&) = delete;

		~Input();

		std::unordered_map<int, bool>& GetKeyMap();
		std::unordered_map<int, bool>& GetMouseMap();
	private:

		static Input* instance;

		Input() {};

		std::unordered_map<int, bool> keymap;
		std::unordered_map<int, bool> prevkeymap;
		std::unordered_map<int, bool> mousemap;
		std::unordered_map<int, bool> prevmousemap;

		glm::vec2* mousePos = nullptr;
		glm::vec2* prevMousePos = nullptr;
	};
}