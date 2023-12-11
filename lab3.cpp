#include <SFML/Graphics.hpp>
#include <functional>
#include <imgui-SFML.h>
#include <imgui.h>
#include <iostream>

enum Shapes
{
	Nx,
	Ny,
	Nz,
	Nw
};

struct Functions
{
	static float RAnd(float w1, float w2) { return w1 + w2 + std::sqrt((w1 * w1 + w2 * w2)); }

	static float ROr(float w1, float w2) { return w1 + w2 - std::sqrt((w1 * w1 + w2 * w2)); }
};

std::vector<sf::VertexArray> gradientPaths;
float **nxMatrix;
float **nyMatrix;

float figure(const sf::Vector2f &point)
{
	std::function<float(const sf::Vector2f &)> rFuncs[6];

	rFuncs[0] = [](const sf::Vector2f &point) -> float
	{ return std::pow(point.x - 0, 2) + std::pow(point.y + 4, 2) - 4; };
	rFuncs[1] = [](const sf::Vector2f &point) -> float
	{ return std::pow(point.x + 3, 2) + std::pow(point.y - 0, 2) - 4; };
	rFuncs[2] = [](const sf::Vector2f &point) -> float
	{ return std::pow(point.x - 8, 2) + std::pow(point.y - 0, 2) - 6; };
	rFuncs[3] = [](const sf::Vector2f &point) -> float
	{ return std::pow(point.x + 6, 2) + std::pow(point.y - 0, 2) - 1; };
	rFuncs[4] = [](const sf::Vector2f &point) -> float
	{ return std::pow(point.x - 10, 2) + std::pow(point.y - 0, 2) - 9; };
	rFuncs[5] = [](const sf::Vector2f &point) -> float
	{ return std::pow(point.x - 9, 2) + std::pow(point.y - 0, 2) - 9; };


	float res1 = Functions::ROr(rFuncs[0](point), rFuncs[1](point));
	float res2 = Functions::ROr(res1, rFuncs[2](point));
	float res3 = Functions::ROr(res2, rFuncs[3](point));
	float res4 = Functions::RAnd(rFuncs[4](point), rFuncs[5](point));
	float res = Functions::ROr(res3, res4);
	return res;
}


class RFuncSprite : public sf::Sprite
{
public:
	void create(const sf::Vector2u &size, Shapes shapeType);
	void drawRFunc(const std::function<float(const sf::Vector2f &)> &rfunc, const sf::FloatRect &subSpace);
	const sf::Texture &getTexture() const { return _texture; }
	const sf::Image &getImage() const { return _image; }
	void saveToFile(const std::string &filename) { _image.saveToFile(filename); }
	sf::VertexArray gradientDescent(const std::function<float(const sf::Vector2f &)> &rfunc,
									const sf::Vector2f &startPoint, float stepSize = 10);
	void clearGradients();

private:
	sf::Texture _texture;
	sf::Image _image;
	sf::Color _firstColor = sf::Color::Black;
	sf::Color _secondColor = sf::Color::White;
	Shapes _shapeType;

	float calculateDeterminant(const std::vector<std::vector<float>> &matrix);
	sf::Color interpolateColors(const sf::Color &colorFirst, const sf::Color &colorSec, float k);
	sf::Color getPixelColor(float Ai, float Bi, float Ci, float Di, unsigned x, unsigned y);
	sf::Vector2f getGradient(const std::function<float(const sf::Vector2f &)> &rfunc, const sf::Vector2f &point);
};

sf::Color RFuncSprite::getPixelColor(float Ai, float Bi, float Ci, float Di, unsigned x, unsigned y)
{
	float nx, ny, nz, nw;

	const float det = std::sqrt(Ai * Ai + Bi * Bi + Ci * Ci + Di * Di);
	nx = Ai / det;
	ny = Bi / det;
	nz = Ci / det;
	nw = Di / det;

	nxMatrix[y][x] = nx;
	nyMatrix[y][x] = ny;

	float shapeValue;

	switch (_shapeType)
	{
	case Nx:
		shapeValue = nx;
		break;
	case Ny:
		shapeValue = ny;
		break;
	case Nz:
		shapeValue = nz;
		break;
	case Nw:
		shapeValue = nw;
		break;
	default:
		throw std::runtime_error("Wrong shape");
	}

	return interpolateColors(_firstColor, _secondColor, (1.f + shapeValue) / 2);
}

void RFuncSprite::clearGradients()
{ gradientPaths.clear(); }

float RFuncSprite::calculateDeterminant(const std::vector<std::vector<float>> &matrix)
{
	return matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
		matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
		matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
}

void RFuncSprite::create(const sf::Vector2u &size, Shapes shapeType)
{
	_image.create(size.x, size.y, sf::Color::Cyan);
	_texture.loadFromImage(_image);
	setTexture(_texture);

	_shapeType = shapeType;
}

sf::Color RFuncSprite::interpolateColors(const sf::Color &colorFirst, const sf::Color &colorSec, float k)
{
	sf::Color val;
	val.r = static_cast<sf::Uint8>(colorFirst.r + (colorSec.r - colorFirst.r) * k);
	val.g = static_cast<sf::Uint8>(colorFirst.g + (colorSec.g - colorFirst.g) * k);
	val.b = static_cast<sf::Uint8>(colorFirst.b + (colorSec.b - colorFirst.b) * k);
	val.a = static_cast<sf::Uint8>(colorFirst.a + (colorSec.a - colorFirst.a) * k);

	return val;
}

void RFuncSprite::drawRFunc(const std::function<float(const sf::Vector2f &)> &rfunc, const sf::FloatRect &subSpace)
{	
	nxMatrix = new float *[_image.getSize().y];
	nyMatrix = new float *[_image.getSize().y];
	for (int i = 0; i < _image.getSize().y; ++i)
	{
		nxMatrix[i] = new float[_image.getSize().x];
		nyMatrix[i] = new float[_image.getSize().x];
	}

	const sf::Texture &texture = getTexture();

	sf::Vector2f spaceStep = {
		subSpace.width / static_cast<float>(_image.getSize().x),
		subSpace.height / static_cast<float>(_image.getSize().y),
	};

	for (unsigned y = 0; y < _image.getSize().y; ++y)
	{
		for (unsigned x = 0; x < _image.getSize().x; ++x)
		{
			int x1 = x;
			int y1 = y;
			int x2 = x + 1;
			int y2 = y;
			int x3 = x;
			int y3 = y + 1;

			sf::Vector2f spacePoint1 = {subSpace.left + static_cast<float>(x1) * spaceStep.x,
										subSpace.top + static_cast<float>(y1) * spaceStep.y};


			sf::Vector2f spacePoint2 = {subSpace.left + static_cast<float>(x2) * spaceStep.x,
										subSpace.top + static_cast<float>(y2) * spaceStep.y};


			sf::Vector2f spacePoint3 = {subSpace.left + static_cast<float>(x3) * spaceStep.x,
										subSpace.top + static_cast<float>(y3) * spaceStep.y};


			const float z1 = rfunc(spacePoint1);
			const float z2 = rfunc(spacePoint2);
			const float z3 = rfunc(spacePoint3);

			const float Ai = calculateDeterminant({
				{spacePoint1.y, z1, 1},
				{spacePoint2.y, z2, 1},
				{spacePoint3.y, z3, 1},
			});

			const float Bi = calculateDeterminant({
				{spacePoint1.x, z1, 1},
				{spacePoint2.x, z2, 1},
				{spacePoint3.x, z3, 1},
			});

			const float Ci = calculateDeterminant({
				{spacePoint1.x, spacePoint1.y, 1},
				{spacePoint2.x, spacePoint2.y, 1},
				{spacePoint3.x, spacePoint3.y, 1},
			});

			const float Di = calculateDeterminant({
				{spacePoint1.x, spacePoint1.y, z1},
				{spacePoint2.x, spacePoint2.y, z2},
				{spacePoint3.x, spacePoint3.y, z3},
			});

			_image.setPixel(x, y, getPixelColor(Ai, Bi, Ci, Di, x, y));
		}
	}

	_texture.loadFromImage(_image);
}

sf::Vector2f RFuncSprite::getGradient(const std::function<float(const sf::Vector2f &)> &rfunc, const sf::Vector2f &point)
{
	unsigned x = point.x;
	unsigned y = point.y;

	return sf::Vector2f(nxMatrix[y][x], nyMatrix[y][x]);
}

sf::VertexArray RFuncSprite::gradientDescent(const std::function<float(const sf::Vector2f &)> &rfunc,
										   const sf::Vector2f &startPoint, float stepSize)
{
	sf::VertexArray descentPath(sf::LineStrip);
	std::vector<sf::Vertex> vertexList;
	auto vertex = sf::Vertex(startPoint, sf::Color::Green);
	descentPath.append(vertex);
	vertexList.push_back(vertex);

	sf::Vector2f currentPoint = startPoint;

	while (true)
	{
		sf::Vector2f gradient = getGradient(rfunc, currentPoint);
		currentPoint += stepSize * gradient;

		if (currentPoint.x >= _image.getSize().x || currentPoint.y >= _image.getSize().y || currentPoint.x <= 0 ||
			currentPoint.y <= 0)
		{
			return descentPath;
		}

		for (int i = 0; i < vertexList.size(); i++)
		{
			unsigned x1 = currentPoint.x;
			unsigned y1 = currentPoint.y; 
			unsigned x2 = vertexList[i].position.x;
			unsigned y2 = vertexList[i].position.y;
			
			if (x1 == x2 && y1 == y2)
			{
				return descentPath;		
			}
		}

		auto tempVertex = sf::Vertex(currentPoint, sf::Color::Green);
		descentPath.append(tempVertex);
		vertexList.push_back(tempVertex);
	}

	return descentPath;
}


void HandleUserInput(sf::RenderWindow &window, const sf::Event &event, RFuncSprite &rFunc)
{
	switch (event.type)
	{
	case sf::Event::Closed:
		window.close();
		break;
	case sf::Event::MouseButtonPressed:
	{
		sf::Vector2f mouseClickPoint;
		sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
		mouseClickPoint = window.mapPixelToCoords(mousePosition);

		sf::VertexArray descentPathNx =
			rFunc.gradientDescent([](const sf::Vector2f &point) -> float { return figure(point); }, mouseClickPoint);

		gradientPaths.push_back(descentPathNx);
	}
	default:
		break;
	}
}

void renderGui(sf::RenderWindow &window, RFuncSprite &rFuncSpriteNx)
{
	ImGui::Begin("Settings");

	if (ImGui::Button("Save nx"))
	{
		rFuncSpriteNx.saveToFile("nx.png");
	}

	if (ImGui::Button("Clear gradients"))
	{
		rFuncSpriteNx.clearGradients();
	}

	ImGui::End();
}

void drawAllSprites(sf::RenderWindow &window, RFuncSprite arr[], int arrSize)
{
	window.clear();
	for (int i = 0; i < arrSize; i++)
	{
		window.draw(arr[i]);
	}
}

int main()
{
	const int width = 600;
	const int height = 600;
	sf::RenderWindow window(sf::VideoMode(width, height), "Lab2");


	window.setFramerateLimit(60);
	if (!ImGui::SFML::Init(window))
	{
		std::cout << "ImGui initialization failed\n";
		return -1;
	}


	sf::Vector2u spriteSize = window.getSize();

	RFuncSprite rFuncSpriteNx;
	rFuncSpriteNx.create(spriteSize, Shapes::Nx);

	sf::FloatRect subSpace(-10.f, -10.f, 20.f, 20.f);
	RFuncSprite arr[1] = {rFuncSpriteNx};
	rFuncSpriteNx.drawRFunc(&figure, subSpace);


	sf::Clock deltaClock;
	while (window.isOpen())
	{
		sf::Event event;

		while (window.pollEvent(event))
		{
			ImGui::SFML::ProcessEvent(window, event);
			HandleUserInput(window, event, rFuncSpriteNx);
		}
		ImGui::SFML::Update(window, deltaClock.restart());

		renderGui(window, rFuncSpriteNx);

		drawAllSprites(window, arr, 1);

		for (const auto &gradientPath : gradientPaths)
		{
			window.draw(gradientPath);
		}

		ImGui::SFML::Render(window);

		window.display();
	}
	ImGui::SFML::Shutdown();

	return 0;
}