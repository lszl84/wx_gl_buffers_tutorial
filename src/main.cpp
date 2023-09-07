#include <wx/wx.h>

#include <GL/glew.h> // must be included before glcanvas.h
#include <wx/glcanvas.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "surface.h"

class MyApp : public wxApp
{
public:
    MyApp() {}
    bool OnInit() wxOVERRIDE;
};

class OpenGLCanvas;

class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString &title);

private:
    OpenGLCanvas *openGLCanvas{nullptr};
};

class OpenGLCanvas : public wxGLCanvas
{
public:
    OpenGLCanvas(MyFrame *parent, const wxGLAttributes &canvasAttrs);
    ~OpenGLCanvas();

    bool InitializeOpenGLFunctions();
    bool InitializeOpenGL();

    void OnPaint(wxPaintEvent &event);
    void OnSize(wxSizeEvent &event);

    void OnKeyDown(wxKeyEvent &event);

private:
    wxGLContext *openGLContext;
    bool isOpenGLInitialized{false};

    float anglez{0.0f};
    float anglex{0.0f};

    Surface s{&f};

    static float f(float x, float y)
    {
        constexpr float epsilon = 0.0001f;
        constexpr float k = 10.0f;
        if (std::abs(x) < epsilon && std::abs(y) < epsilon)
            return 1.0f;
        else
            return sin(sqrt(pow(x * k, 2) + pow(y * k, 2))) / sqrt(pow(x * k, 2) + pow(y * k, 2)); // sombrero equation
    }

    unsigned int VAO, VBO, EBO, shaderProgram;
};

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    MyFrame *frame = new MyFrame("Hello OpenGL");
    frame->Show(true);

    return true;
}

MyFrame::MyFrame(const wxString &title)
    : wxFrame(nullptr, wxID_ANY, title)
{
    wxGLAttributes vAttrs;
    vAttrs.PlatformDefaults().Defaults().EndList();

    if (wxGLCanvas::IsDisplaySupported(vAttrs))
    {
        auto sizer = new wxBoxSizer(wxHORIZONTAL);

        openGLCanvas = new OpenGLCanvas(this, vAttrs);
        openGLCanvas->SetMinSize(FromDIP(wxSize(600, 600)));

        sizer->Add(openGLCanvas, 1, wxEXPAND);
        SetSizerAndFit(sizer);
    }
}

OpenGLCanvas::OpenGLCanvas(MyFrame *parent, const wxGLAttributes &canvasAttrs)
    : wxGLCanvas(parent, canvasAttrs)
{
    wxGLContextAttrs ctxAttrs;
    ctxAttrs.PlatformDefaults().CoreProfile().OGLVersion(3, 3).EndList();
    openGLContext = new wxGLContext(this, nullptr, &ctxAttrs);

    if (!openGLContext->IsOK())
    {
        wxMessageBox("This sample needs an OpenGL 3.3 capable driver.",
                     "OpenGL version error", wxOK | wxICON_INFORMATION, this);
        delete openGLContext;
        openGLContext = nullptr;
    }

    Bind(wxEVT_PAINT, &OpenGLCanvas::OnPaint, this);
    Bind(wxEVT_SIZE, &OpenGLCanvas::OnSize, this);

    Bind(wxEVT_KEY_DOWN, &OpenGLCanvas::OnKeyDown, this);
}

OpenGLCanvas::~OpenGLCanvas()
{
    delete openGLContext;
}

bool OpenGLCanvas::InitializeOpenGLFunctions()
{
    GLenum err = glewInit();

    if (GLEW_OK != err)
    {
        wxLogError("OpenGL GLEW initialization failed: %s", reinterpret_cast<const char *>(glewGetErrorString(err)));
        return false;
    }

    wxLogDebug("Status: Using GLEW %s", reinterpret_cast<const char *>(glewGetString(GLEW_VERSION)));

    return true;
}

bool OpenGLCanvas::InitializeOpenGL()
{
    if (!openGLContext)
    {
        return false;
    }

    SetCurrent(*openGLContext);

    if (!InitializeOpenGLFunctions())
    {
        wxMessageBox("Error: Could not initialize OpenGL function pointers.",
                     "OpenGL initialization error", wxOK | wxICON_INFORMATION, this);
        return false;
    }

    wxLogDebug("OpenGL version: %s", reinterpret_cast<const char *>(glGetString(GL_VERSION)));
    wxLogDebug("OpenGL vendor: %s", reinterpret_cast<const char *>(glGetString(GL_VENDOR)));

    constexpr auto vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        uniform mat4 modelMatrix;

        void main()
        {
            gl_Position = modelMatrix * vec4(aPos.x, aPos.y, aPos.z, 1.0);
        }
    )";

    constexpr auto fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    )";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        wxLogDebug("Vertex Shader Compilation Failed: %s", infoLog);
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        wxLogDebug("Fragment Shader Compilation Failed: %s", infoLog);
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        wxLogDebug("Shader Program Linking Failed: %s", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, s.xyzArray.size() * sizeof(float), s.xyzArray.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, s.lineEndpointIndices.size() * sizeof(unsigned int), s.lineEndpointIndices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    isOpenGLInitialized = true;
    return true;
}

void OpenGLCanvas::OnPaint(wxPaintEvent &WXUNUSED(event))
{
    wxPaintDC dc(this);

    if (!isOpenGLInitialized)
    {
        return;
    }

    SetCurrent(*openGLContext);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);

    auto modelMatrix = glm::mat4(1.0f);

    modelMatrix = glm::rotate(modelMatrix, glm::radians(anglex), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(anglez), glm::vec3(0.0f, 0.0f, 1.0f));

    auto modelMatrixLocation = glGetUniformLocation(shaderProgram, "modelMatrix");
    glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));

    glBindVertexArray(VAO);
    glDrawElements(GL_LINES, s.lineEndpointIndices.size(), GL_UNSIGNED_INT, 0);

    SwapBuffers();
}

void OpenGLCanvas::OnSize(wxSizeEvent &event)
{
    bool firstApperance = IsShownOnScreen() && !isOpenGLInitialized;

    if (firstApperance)
    {
        InitializeOpenGL();
    }

    if (isOpenGLInitialized)
    {
        auto viewPortSize = event.GetSize() * GetContentScaleFactor();
        glViewport(0, 0, viewPortSize.x, viewPortSize.y);
    }

    event.Skip();
}

void OpenGLCanvas::OnKeyDown(wxKeyEvent &event)
{
    constexpr float AngleIncrement = 5.0f;

    if (event.GetKeyCode() == WXK_LEFT)
    {
        anglez -= AngleIncrement;
    }
    else if (event.GetKeyCode() == WXK_RIGHT)
    {
        anglez += AngleIncrement;
    }
    else if (event.GetKeyCode() == WXK_UP)
    {
        anglex += AngleIncrement;
    }
    else if (event.GetKeyCode() == WXK_DOWN)
    {
        anglex -= AngleIncrement;
    }
    else
    {
        event.Skip();
    }

    Refresh();
}