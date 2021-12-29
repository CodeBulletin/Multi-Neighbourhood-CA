#include "App.hpp"
#include "sfhelper.hpp"
#include "imgui/imgui_stdlib.h"
#include "imgui/imgui.h"

#include <opencv2/opencv.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

#include <Windows.h>

void HideConsole()
{
	::ShowWindow(::GetConsoleWindow(), 0);
}

void ShowConsole()
{
	::ShowWindow(::GetConsoleWindow(), 1);
}

bool IsConsoleVisible()
{
	return ::IsWindowVisible(::GetConsoleWindow()) != 0;
}

struct Rule {
	sf::Vector2i Range;
	sf::Vector2f AliveRange, DeadRange;
};

struct Rules {
	std::vector<Rule> rules;
	std::string name;
};

struct AnimationRules {
	std::vector<Rule> rules;
	std::string name;
	float col1[4];
	float t_end;
};

class MNCA : public App {
public:
	std::string guiPath, fileName;
	std::vector<Rule> rules;
	std::vector<Rules> rulesBuffer;
	std::vector<AnimationRules> animationRuleBuffer;
	
	bool vSync = true;
	bool PlayPause = false;
	bool showRulesBuffer = false;
	bool animation_ui = false, animate = false;
	bool showAllFiles = false;
	bool showSettings = false;
	bool showInfo = false;
	
	int ruleNum = 0;
	int numRules = 8, numNewRules = 8;
	int frameRate;
	int x_Width = 764, y_Height = 432;
	int x_nWidth = 764, y_nHeight = 432;
	int cPoint = 25;

	sf::RenderTexture Tex;
	sf::Texture T;
	sf::Sprite drawSprite1;
	sf::RectangleShape drawRect1;
	sf::Shader noise, mnca, flipChannel, threshold, normalize, blur1, blur2, swizzel;

	std::string mnca_buffer;

	float x_Scale = 2.5f, y_Scale = 2.5f;
	float x_Shift = 0, y_Shift = 0;
	float cRad, cRadMin, cRadMax;

	float col1[4], colc1[4];

	float t;
	int animation_index;


	MNCA() {
		name = "Multi Neighbourhood Cellular Automaton";
		load_ini();
		create_window();

		window.setVerticalSyncEnabled(vSync);
		window.setFramerateLimit(frameRate);

		auto path = std::filesystem::current_path() / "Assets\\icon.png";
		sf::Image icon;
		icon.loadFromFile(path.string());
		window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
	}

	void load_ini() {
		std::fstream fs;
		auto path = std::filesystem::current_path() / "Assets\\settings.ini";
		fs.open(path);
		fs >> vSync;
		fs >> frameRate;
		fs >> x_Width;
		fs >> y_Height;
		fs >> cRadMin;
		fs >> cRadMax;
		fs >> cPoint;

		x_nWidth = x_Width;
		y_nHeight = y_Height;
	}

	void setup() override {

		x_Scale = width / (float)x_Width;
		y_Scale = height / (float)y_Height;

		cRad = cRadMin;
		auto path = std::filesystem::current_path();
		guiPath = (path / "Assets/gui.ini").string();
		ImGui::GetIO().IniFilename = guiPath.c_str();
		setRandomRules();

		std::fstream file;
		file.open("Assets/Shader/mnca.glsl");
		std::stringstream buffer;
		buffer << file.rdbuf();
		mnca_buffer = buffer.str();

		noise.loadFromFile("Assets/Shader/noise.glsl", sf::Shader::Type::Fragment);
		flipChannel.loadFromFile("Assets/Shader/flipChannel.glsl", sf::Shader::Type::Fragment);

		threshold.loadFromFile("Assets/Shader/threshold.glsl", sf::Shader::Type::Fragment);

		swizzel.loadFromFile("Assets/Shader/swizzel.glsl", sf::Shader::Type::Fragment);

		normalize.loadFromFile("Assets/Shader/normalize.glsl", sf::Shader::Type::Fragment);

		blur1.loadFromFile("Assets/Shader/blur.glsl", sf::Shader::Type::Fragment);
		sf::Vector2f vec1(0, 0.25);
		blur1.setUniform("rad", vec1);

		blur2.loadFromFile("Assets/Shader/blur.glsl", sf::Shader::Type::Fragment);
		sf::Vector2f vec2(0.25, 0);
		blur2.setUniform("rad", vec2);

		setWidthHeight();

		drawRect1.setSize({ (float)x_Width, (float)y_Height });
		Tex.create(x_Width, y_Height, window.getSettings());

		CreateNoise({x_Shift, y_Shift});

		T = Tex.getTexture();

		drawSprite1.setTexture(Tex.getTexture());
		drawSprite1.scale(x_Scale, y_Scale);

		setmnca();

		t = 0;
		animation_index = 0;

		random_color(col1);
	}

	void setWidthHeight() {
		flipChannel.setUniform("Width", (float)x_Width);
		flipChannel.setUniform("Height", (float)y_Height);
		threshold.setUniform("Width", (float)x_Width);
		threshold.setUniform("Height", (float)y_Height);
		swizzel.setUniform("Width", (float)x_Width);
		swizzel.setUniform("Height", (float)y_Height);
		normalize.setUniform("Width", (float)x_Width);
		normalize.setUniform("Height", (float)y_Height);
		blur1.setUniform("Width", (float)x_Width);
		blur1.setUniform("Height", (float)y_Height);
		blur2.setUniform("Width", (float)x_Width);
		blur2.setUniform("Height", (float)y_Height);
	}

	void random_color(float* col) {
		auto a = sfh::random::random3d(0.f, 1.f);

		col[0] = a.x;
		col[1] = a.y;
		col[2] = a.z;
		col[3] = 1.f;
	}

	void setmnca() {
		std::string mnca_buffer_copy = mnca_buffer;
		mnca_buffer_copy.replace(mnca_buffer_copy.find("<num>"), 5, std::to_string(numRules * 2));
		mnca.loadFromMemory(mnca_buffer_copy, sf::Shader::Type::Fragment);
		mnca.setUniform("Width", (float)x_Width);
		mnca.setUniform("Height", (float)y_Height);

		for (int i = 0; i < numRules; i++) {
			std::string var = "rules[" + std::to_string(i) + "]";
			mnca.setUniform(var + ".range", rules[i].Range);
			mnca.setUniform(var + ".aliveRange", rules[i].AliveRange);
			mnca.setUniform(var + ".deadRange", rules[i].DeadRange);
		}
	}

	void setRandomRules() {
		numRules = 2 * numNewRules;
		rules.clear();
		for (int i = 0; i < numRules; i++) {
			Rule rule;
			int a = sfh::random::random1d<int>(0, 10);
			int b = sfh::random::random1d<int>(0, 10);
			rule.Range.x = std::min(a, b);
			rule.Range.y = std::max(a, b);
			float aa = sfh::random::random1d(0.0f, 1.0f);
			float ab = sfh::random::random1d(0.0f, 1.0f);
			rule.AliveRange.x = std::min(aa, ab);
			rule.AliveRange.y = std::max(aa, ab);
			float da = sfh::random::random1d(0.0f, 1.0f);
			float db = sfh::random::random1d(0.0f, 1.0f);
			rule.DeadRange.x = std::min(da, db);
			rule.DeadRange.y = std::max(da, db);
			rules.push_back(rule);
		}
		rulesBuffer.push_back({ rules, std::to_string(ruleNum) });
		ruleNum++;
	}

	void CreateNoise(sf::Vector2f shift) {
		noise.setUniform("displacement", shift);
		Tex.clear();
		Tex.draw(drawRect1, &noise);
		Tex.display();
	}

	void multi_neighbourhood_CA() {

		mnca.setUniform("tex", T);
		Tex.clear();
		Tex.draw(drawRect1, &mnca);

		if (!ImGui::GetIO().WantCaptureMouse || !GUI) {
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
				sf::Vector2i position = sf::Mouse::getPosition();
				sf::Vector2f pos = sf::Vector2f(position.x / (float)width * x_Width, position.y / (float)height * y_Height);
				sf::CircleShape cs;
				cs.setRadius(cRad);
				cs.setFillColor(sf::Color::White);
				cs.setOrigin(cRad, cRad);
				cs.setPosition(pos);
				cs.setPointCount(cPoint);
				Tex.draw(cs);
			}
			else if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
				sf::Vector2i position = sf::Mouse::getPosition();
				sf::Vector2f pos = sf::Vector2f(position.x / (float)width * x_Width, position.y / (float)height * y_Height);
				sf::CircleShape cs;
				cs.setRadius(cRad);
				cs.setFillColor(sf::Color::Black);
				cs.setOrigin(cRad, cRad);
				cs.setPosition(pos);
				cs.setPointCount(cPoint);
				Tex.draw(cs);
			}
		}

		Tex.display();
		T = Tex.getTexture();

		flipChannel.setUniform("tex", T);
		Tex.clear();
		Tex.draw(drawRect1, &flipChannel);
		Tex.display();
		T = Tex.getTexture();

		threshold.setUniform("tex", T);
		Tex.clear();
		Tex.draw(drawRect1, &threshold);
		Tex.display();

		blur1.setUniform("tex", Tex.getTexture());
		Tex.clear();
		Tex.draw(drawRect1, &blur1);
		Tex.display();

		blur2.setUniform("tex", Tex.getTexture());
		Tex.clear();
		Tex.draw(drawRect1, &blur2);
		Tex.display();

		sf::Image img = Tex.getTexture().copyToImage();
		sf::Vector2u siz = img.getSize();
		uchar* pixels = (uchar*)img.getPixelsPtr();
		cv::Mat mat(cv::Size(siz.x, siz.y), CV_8UC4, pixels);

		cv::cvtColor(mat, mat, cv::COLOR_RGB2GRAY);

		double min, max;
		cv::minMaxIdx(mat, &min, &max);
		float min_val = float(min) / 255.0f;
		float max_val = float(max) / 255.0f;

		normalize.setUniform("min_val", min_val);
		normalize.setUniform("max_val", max_val);
		normalize.setUniform("tex", Tex.getTexture());
		Tex.clear();
		Tex.draw(drawRect1, &normalize);
		Tex.display();

		sf::Glsl::Vec4 v1 = { colc1[0], colc1[1], colc1[2], colc1[3]};

		swizzel.setUniform("c", v1);

		swizzel.setUniform("tex", Tex.getTexture());
		Tex.clear();
		Tex.draw(drawRect1, &swizzel);
		Tex.display();

		drawSprite1.setTexture(Tex.getTexture());
	}

	void copy_col(float* colc, float* col) {
		colc[0] = col[0];
		colc[1] = col[1];
		colc[2] = col[2];
		colc[3] = col[3];
	}

	void reset() {
		CreateNoise({ x_Shift, y_Shift });

		T = Tex.getTexture();

		t = 0;

		animation_index = 0;

		if (animate) {
			if (animationRuleBuffer.size() == 0) {
				animate = false;
			}
			else {
				rules = animationRuleBuffer[animation_index].rules;
				copy_col(colc1, animationRuleBuffer[animation_index].col1);
				setmnca();
			}
		}
	}

	static inline bool is_not_alnum_space(char c)
	{
		return !isalnum(c);
	}

	bool valid(const std::string& str) {
		return std::find_if(str.begin(), str.end(), is_not_alnum_space) == str.end();
	}

	void save() {
		if (fileName.size() > 0 && valid(fileName)) {
			auto path = std::filesystem::current_path() / ("Saved Files\\Rules\\"+fileName);
			std::filesystem::create_directory(path);

			auto file1 = path / (fileName + ".ini");
			auto file2 = path / (fileName + ".rules");

			std::fstream fs;
			fs.open(file1, std::ios_base::in | std::ios_base::out | std::ios_base::trunc);
			fs << col1[0] << '\n';
			fs << col1[1] << '\n';
			fs << col1[2] << '\n';
			fs << col1[3] << '\n';
			fs << numRules << '\n';
			fs << x_Shift << '\n';
			fs << y_Shift << '\n';
			fs.close();

			fs.open(file2, std::ios_base::in | std::ios_base::out | std::ios_base::trunc);
			for (auto i : rules) {
				fs << i.Range.x << '\n';
				fs << i.Range.y << '\n';
				fs << i.AliveRange.x << '\n';
				fs << i.AliveRange.y << '\n';
				fs << i.DeadRange.x << '\n';
				fs << i.DeadRange.y << '\n';
			}
			fs.close();
		}
	}

	void Load() {
		if (fileName.size() > 0 && valid(fileName)) {
			auto path = std::filesystem::current_path() / ("Saved Files\\Rules\\" + fileName);

			auto file1 = path / (fileName + ".ini");
			auto file2 = path / (fileName + ".rules");

			std::fstream fs;
			fs.open(file1);
			fs >> col1[0];
			fs >> col1[1];
			fs >> col1[2];
			fs >> col1[3];
			fs >> numRules;
			fs >> x_Shift;
			fs >> y_Shift;
			fs.close();

			numNewRules = numRules / 2;

			fs.open(file2);
			rules.clear();
			for (int i = 0; i < numRules; i++) {
				Rule rule;
				fs >> rule.Range.x;
				fs >> rule.Range.y;
				fs >> rule.AliveRange.x;
				fs >> rule.AliveRange.y;
				fs >> rule.DeadRange.x;
				fs >> rule.DeadRange.y;
				rules.push_back(rule);
			}

			setmnca();
			reset();

			rulesBuffer.push_back({ rules, fileName });
			ruleNum++;
		}
	}

	void loadFromBuffer(int i) {
		rules = rulesBuffer[i].rules;
		numRules = rulesBuffer[i].rules.size();
		numNewRules = numRules / 2;
	}

	void deleteFromBuffer(int i) {
		rulesBuffer.erase(rulesBuffer.begin() + i);
	}

	void addToAnimationBuffer(int i) {
		AnimationRules rule;
		rule.rules = rulesBuffer[i].rules;
		rule.name = rulesBuffer[i].name;
		copy_col(rule.col1, col1);
		rule.t_end = 0;
		animationRuleBuffer.push_back(rule);
	}

	void ApplyChanges() {
		setmnca();
	}

	void loop() override {
		window.clear(sf::Color::Black);

		if (PlayPause) {
			if (animate) {
				t += frameTime;
				std::cout << t << "\n";
				if (t > animationRuleBuffer[animation_index].t_end) {
					animation_index++;
					if (animation_index == animationRuleBuffer.size()) {
						reset();
					}
					else {
						std::cout << "hello\n";
						rules = animationRuleBuffer[animation_index].rules;
						copy_col(colc1, animationRuleBuffer[animation_index].col1);
						setmnca();
					}
					t = 0;
				}
				multi_neighbourhood_CA();
			}
			else {
				copy_col(colc1, col1);
				multi_neighbourhood_CA();
			}
		}
		window.draw(drawSprite1);
	}

	void gui() override {
		ImGui::Begin(name.c_str());
		ImGui::InputText("Name", &fileName);  ImGui::SameLine();
		if (ImGui::Button("save")) save();  ImGui::SameLine();
		if (ImGui::Button("Load")) Load();
		ImGui::SliderInt("No of Rules * 2", &numNewRules, 1, 8);

		if (ImGui::Button("New Rules")) setRandomRules();  ImGui::SameLine();
		if (ImGui::Button("Apply Changes")) ApplyChanges();  ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			reset();
		}
		ImGui::SameLine();
		if (ImGui::Button("Randomize")) {
			setRandomRules();
			random_color(col1);
			CreateNoise({ x_Shift, y_Shift });
			T = Tex.getTexture();
			ApplyChanges();
		}
		ImGui::SameLine();
		if (ImGui::Button("Buffer")) {
			showRulesBuffer = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("animation")) {
			animation_ui = true;
		}
		if (ImGui::Button("Save To Buffer")) {
			rulesBuffer.push_back({ rules, std::to_string(ruleNum) });
			ruleNum++;
		}
		if (ImGui::Button("Show All Files")) {
			showAllFiles = true;
		}
		if (ImGui::Button("Settings")) {
			showSettings = true;
		} ImGui::SameLine();

		if (ImGui::Button("Info and Help")) {
			showInfo = true;
		}

		if (ImGui::SliderFloat("x shift", &x_Shift, 0, 1000) ||
			ImGui::SliderFloat("y shift", &y_Shift, 0, 1000)) {
			reset();
		}

		ImGui::ColorEdit3("X", col1);

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();

		if (!animate) {
			ImGui::BeginChild("child", { 0, 0 }, false, ImGuiWindowFlags_HorizontalScrollbar);
			ImGui::TextUnformatted("Rules: ");
			for (int i = 0; i < numRules; i++) {
				std::string name = "Rule " + std::to_string(i + 1);
				ImGui::BeginGroup();
				ImGui::BulletText(name.c_str());
				name = "Range Min##" + std::to_string(i + 1);
				ImGui::SliderInt(name.c_str(), &rules[i].Range.x, 0, 16);
				name = "Range Max##" + std::to_string(i + 1);
				ImGui::SliderInt(name.c_str(), &rules[i].Range.y, rules[i].Range.x, 16);

				name = "Alive Range Min##" + std::to_string(i + 1);
				ImGui::SliderFloat(name.c_str(), &rules[i].AliveRange.x, 0.f, 1.f);
				name = "Alive Range Max##" + std::to_string(i + 1);
				ImGui::SliderFloat(name.c_str(), &rules[i].AliveRange.y, rules[i].AliveRange.x, 1.f);

				name = "Dead Range Min##" + std::to_string(i + 1);
				ImGui::SliderFloat(name.c_str(), &rules[i].DeadRange.x, 0, 1.f);
				name = "Dead Range Max##" + std::to_string(i + 1);
				ImGui::SliderFloat(name.c_str(), &rules[i].DeadRange.y, rules[i].DeadRange.x, 1.f);
				ImGui::EndGroup();
			}
			ImGui::EndChild();
		}
		ImGui::End();

		if (showRulesBuffer) {
			ImGui::Begin("Buffer");
			if (ImGui::Button("Close")) showRulesBuffer = false; ImGui::SameLine();
			if (ImGui::Button("Clear")) rulesBuffer.clear();
			ImGui::BeginChild("child", { 0, 0 }, false, ImGuiWindowFlags_HorizontalScrollbar);
			int i = 0;
			for (auto& rule : rulesBuffer) {
				std::string name = "##" + std::to_string(i + 1);
				std::string load = "load##" + std::to_string(i + 1);
				std::string delete_ = "delete##" + std::to_string(i + 1);
				std::string animate = "Add to Animation##" + std::to_string(i + 1);
				ImGui::InputText(name.c_str(), &rule.name); ImGui::SameLine();
				if (ImGui::Button(load.c_str())) loadFromBuffer(i);
				ImGui::SameLine();
				if (ImGui::Button(delete_.c_str())) deleteFromBuffer(i);
				ImGui::SameLine();
				if (ImGui::Button(animate.c_str())) addToAnimationBuffer(i);
				i++;
			}
			ImGui::EndChild();
			ImGui::End();
		}

		if (animation_ui) {
			ImGui::Begin("Animation");
			if (ImGui::Button("Close")) animation_ui = false; ImGui::SameLine();
			if (ImGui::Button("Clear")) {
				if (animate) {
					animate = false;
					reset();
				}
				animationRuleBuffer.clear(); ImGui::SameLine();
			}
			if (ImGui::Checkbox("Animate", &animate)) {
				reset();
			}
			ImGui::BeginChild("child", { 0, 0 }, false, ImGuiWindowFlags_HorizontalScrollbar);
			int i = 0;
			for (auto& rule : animationRuleBuffer) {
				std::string Text = "A" + std::to_string(i + 1) + ":";
				std::string name = "##" + std::to_string(i + 1);
				std::string delete_ = "delete##" + std::to_string(i + 1);
				std::string color = "color##" + std::to_string(i + 1);
				std::string time = "end at(s)##" + std::to_string(i + 1);
				ImGui::Text(Text.c_str());
				ImGui::InputText(name.c_str(), &rule.name); ImGui::SameLine();
				ImGui::SameLine();
				if (ImGui::Button(delete_.c_str())) {
					animationRuleBuffer.erase(animationRuleBuffer.begin() + i);
				}
				ImGui::ColorEdit3(color.c_str(), rule.col1);
				ImGui::InputFloat(time.c_str(), &rule.t_end);
				i++;
			}
			ImGui::EndChild();
			ImGui::End();
		}
		if (showAllFiles) {
			ImGui::Begin("Files");
			if (ImGui::Button("Close")) showAllFiles = false;
			ImGui::BeginChild("child", { 0, 0 }, false, ImGuiWindowFlags_HorizontalScrollbar);
			int i = 0;

			auto path = std::filesystem::current_path() / "Saved Files\\Rules";
			for (const auto& entry : std::filesystem::directory_iterator(path)) {
				ImGui::Text(entry.path().filename().string().c_str());
			}
			ImGui::EndChild();
			ImGui::End();
		}

		if (showSettings) {
			ImGui::Begin("Settings");
			if (ImGui::Button("Close")) showSettings = false; ImGui::SameLine();
			if (ImGui::Button("Save and Apply Settings")) {
				window.setVerticalSyncEnabled(vSync);
				window.setFramerateLimit(frameRate);

				x_Width = x_nWidth;
				y_Height = y_nHeight;

				x_Scale = width / (float)x_Width;
				y_Scale = height / (float)y_Height;

				setWidthHeight();
				setmnca();

				drawRect1.setSize({ (float)x_Width, (float)y_Height });
				Tex.create(x_Width, y_Height, window.getSettings());

				CreateNoise({ x_Shift, y_Shift });

				T = Tex.getTexture();

				drawSprite1.setTexture(Tex.getTexture());
				drawSprite1.setTextureRect(sf::IntRect(0, 0, x_Width, y_Height));
				drawSprite1.setScale(1, 1);
				drawSprite1.scale(x_Scale, y_Scale);

				std::fstream fs;
				auto path = std::filesystem::current_path() / "Assets\\settings.ini";
				fs.open(path, std::ios::in | std::ios::out | std::ios::trunc);
				fs << vSync << "\n";
				fs << frameRate << "\n";
				fs << x_Width << "\n";
				fs << y_Height << "\n";
				fs << cRadMin << "\n";
				fs << cRadMax << "\n";
				fs << cPoint << "\n";
				fs.close();
			}

			ImGui::Checkbox("vSync", &vSync);
			ImGui::InputInt("FrameRate", &frameRate);

			ImGui::InputInt("Width", &x_nWidth);
			ImGui::InputInt("Height", &y_nHeight);

			ImGui::SliderFloat("circle radius min", &cRadMin, 0, x_Width / 2.f);
			ImGui::SliderFloat("circle radius max", &cRadMax, 0, x_Width / 2.f);
			ImGui::SliderInt("sides", &cPoint, 3, 50);
			ImGui::End();
		}

		if (showInfo) {
			ImGui::Begin("Info and Help");
			if (ImGui::Button("Close")) showInfo = false;
			ImGui::Text("Tilde(~) to Enable/Disable GUI");
			ImGui::Text("Space to Play/Pause Simulation");
			ImGui::Text("F5 to Reset Simulation");
			ImGui::Text("In this simulation, the state of each pixel (called a cell) is initially\n\
randomized between 0 and 1. On every frame, the state of each cell is updated based \n\
on the states of all the cells inside various 'neighbourhoods' around it. These\n\
neighboughhoods are ring-shaped, with each ring defined by a min and max radius,\n\
along with two min/max values called 'alive' and 'dead'. Once all the states inside\n\
a ring have been added up, the sum is compared to the ring's alive and dead ranges,\n\
and depending on which range the sum falls into, the cell's state increases or decreases\n\
by some small amount (if the sum is inside neither range, the state remains the same).");
			ImGui::End();
		}
	}

	void KeyPressedEvent() override {
		if (event.key.code == sf::Keyboard::Tilde) {
			GUI = !GUI;
		}
		else if (event.key.code == sf::Keyboard::Space) {
			PlayPause = !PlayPause;
		}
		else if (event.key.code == sf::Keyboard::F5) {
			setmnca();
			reset();
		}
	}

	void MouseWheelScrolledEvent() override {
		cRad += event.mouseWheelScroll.delta;
		cRad = sfh::clamp(cRad, cRadMin, cRadMax);
	}
};

int main() {
	HideConsole();
	MNCA App;
	App.run();
	return 0;
}