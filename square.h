
// our global shader states
struct SquareShaderState {
	GlProgram program;

	// Handles to uniform variables
	GLint h_uVertexScale;
	GLint h_uTex0, h_uTex1;
	GLfloat h_uRatioWidth;
	GLfloat h_uRatioHeight;

	// Handles to vertex attributes
	GLint h_aPosition;
	GLint h_aTexCoord;
};


struct TriangleShaderState {
	GlProgram program;
	// Handles to uniform variables
	GLint h_uVertexScale;
	GLint h_uTex2;
	GLfloat h_uRatioHeight;
	GLfloat h_uRatioWidth;
	GLfloat h_uYmove;
	GLfloat h_uXmove;

	// Handles to vertex attributes
	GLint h_aPosition;
	GLint h_aTexCoord;
	GLfloat h_aColor;
};

static shared_ptr<SquareShaderState> g_squareShaderState;

// our global texture instance
static shared_ptr<GlTexture> g_tex0, g_tex1;
static shared_ptr<TriangleShaderState> g_triangleShaderState;

// our global texture instance
static shared_ptr<GlTexture> g_tex2;

// our global geometries
struct GeometryPX {
	GlArrayObject vao;
	GlBufferObject posVbo, texVbo, colorVbo;
};

static shared_ptr<GeometryPX> g_square;
static shared_ptr<GeometryPX> g_triangle;

// C A L L B A C K S ///////////////////////////////////////////////////


static void drawSquare() {
	// using a VAO is necessary to run on OS X.
	glBindVertexArray(g_square->vao);

	// activate the glsl program
	glUseProgram(g_squareShaderState->program);

	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *g_tex0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, *g_tex1);

	if (g_windowWidth <= g_windowHeight)
	{
		g_RatioHeight = (float)g_windowWidth / (float)g_windowHeight;
		g_RatioWidth = 1;
	}
	else
	{
		g_RatioHeight = 1;
		g_RatioWidth = (float)g_windowHeight / (float)g_windowWidth;
	}
	// set glsl uniform variable  s
	safe_glUniform1i(g_squareShaderState->h_uTex0, 0); // 0 means GL_TEXTURE0
	safe_glUniform1i(g_squareShaderState->h_uTex1, 1); // 1 means GL_TEXTURE1
	safe_glUniform1f(g_squareShaderState->h_uVertexScale, g_objScale);
	safe_glUniform1f(g_squareShaderState->h_uRatioHeight, g_RatioHeight);
	safe_glUniform1f(g_squareShaderState->h_uRatioWidth, g_RatioWidth);

	// bind vertex buffers
	glBindBuffer(GL_ARRAY_BUFFER, g_square->posVbo);
	safe_glVertexAttribPointer(g_squareShaderState->h_aPosition,
		2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, g_square->texVbo);
	safe_glVertexAttribPointer(g_squareShaderState->h_aTexCoord,
		2, GL_FLOAT, GL_FALSE, 0, 0);

	safe_glEnableVertexAttribArray(g_squareShaderState->h_aPosition);
	safe_glEnableVertexAttribArray(g_squareShaderState->h_aTexCoord);

	// draw using 6 vertices, forming two triangles
	glDrawArrays(GL_TRIANGLES, 0, 6);

	safe_glDisableVertexAttribArray(g_squareShaderState->h_aPosition);
	safe_glDisableVertexAttribArray(g_squareShaderState->h_aTexCoord);

	glBindVertexArray(0);

	// check for errors
	checkGlErrors();
}

static void drawTriangle() {
	// using a VAO is necessary to run on OS X.
	glBindVertexArray(g_triangle->vao);

	// activate the glsl program
	glUseProgram(g_triangleShaderState->program);

	// bind textures
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, *g_tex2);

	if (g_windowWidth <= g_windowHeight)
	{
		g_RatioHeight = (float)g_windowWidth / (float)g_windowHeight;
		g_RatioWidth = 1;
	}
	else
	{
		g_RatioHeight = 1;
		g_RatioWidth = (float)g_windowHeight / (float)g_windowWidth;
	}
	// set glsl uniform variables
	safe_glUniform1i(g_triangleShaderState->h_uTex2, 2);
	safe_glUniform1f(g_triangleShaderState->h_uVertexScale, g_objScale);
	safe_glUniform1f(g_triangleShaderState->h_uRatioHeight, g_RatioHeight);
	safe_glUniform1f(g_triangleShaderState->h_uRatioWidth, g_RatioWidth);
	safe_glUniform1f(g_triangleShaderState->h_uXmove, g_Xmove);
	safe_glUniform1f(g_triangleShaderState->h_uYmove, g_Ymove);

	// bind vertex buffers
	glBindBuffer(GL_ARRAY_BUFFER, g_triangle->posVbo);
	safe_glVertexAttribPointer(g_triangleShaderState->h_aPosition,
		2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, g_triangle->texVbo);
	safe_glVertexAttribPointer(g_triangleShaderState->h_aTexCoord,
		2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, g_triangle->colorVbo);
	safe_glVertexAttribPointer(g_triangleShaderState->h_aColor,
		4, GL_FLOAT, GL_FALSE, 0, 0);

	safe_glEnableVertexAttribArray(g_triangleShaderState->h_aPosition);
	safe_glEnableVertexAttribArray(g_triangleShaderState->h_aTexCoord);
	safe_glEnableVertexAttribArray(g_triangleShaderState->h_aColor);

	// draw using 3 vertices
	glDrawArrays(GL_TRIANGLES, 0, 3);

	safe_glDisableVertexAttribArray(g_triangleShaderState->h_aPosition);
	safe_glDisableVertexAttribArray(g_triangleShaderState->h_aTexCoord);
	safe_glDisableVertexAttribArray(g_triangleShaderState->h_aColor);

	glBindVertexArray(0);

	// check for errors
	checkGlErrors();
}



static void loadSquareShader(SquareShaderState& ss) {
	const GLuint h = ss.program; // short hand

	if (!g_Gl2Compatible) {
		readAndCompileShader(ss.program, "shaders/asst1-sq-gl3.vshader", "shaders/asst1-sq-gl3.fshader");
	}
	else {
		readAndCompileShader(ss.program, "shaders/asst1-sq-gl2.vshader", "shaders/asst1-sq-gl2.fshader");
	}

	// Retrieve handles to uniform variables
	ss.h_uVertexScale = safe_glGetUniformLocation(h, "uVertexScale");
	ss.h_uTex0 = safe_glGetUniformLocation(h, "uTex0");
	ss.h_uTex1 = safe_glGetUniformLocation(h, "uTex1");
	ss.h_uRatioHeight = safe_glGetUniformLocation(h, "uRatioHeight");
	ss.h_uRatioWidth = safe_glGetUniformLocation(h, "uRatioWidth");

	// Retrieve handles to vertex attributes
	ss.h_aPosition = safe_glGetAttribLocation(h, "aPosition");
	ss.h_aTexCoord = safe_glGetAttribLocation(h, "aTexCoord");

	if (!g_Gl2Compatible)
		glBindFragDataLocation(h, 0, "fragColor");
	checkGlErrors();
}

static void loadTriangleShader(TriangleShaderState& ss) {
	const GLuint h = ss.program; // short hand

	if (!g_Gl2Compatible) {
		readAndCompileShader(ss.program, "shaders/asst1-tri-gl3.vshader", "shaders/asst1-tri-gl3.fshader");
	}
	else {
		readAndCompileShader(ss.program, "shaders/asst1-tri-gl2.vshader", "shaders/asst1-tri-gl2.fshader");
	}

	// Retrieve handles to uniform variables
	ss.h_uVertexScale = safe_glGetUniformLocation(h, "uVertexScale");
	ss.h_uTex2 = safe_glGetUniformLocation(h, "uTex2");
	ss.h_uRatioHeight = safe_glGetUniformLocation(h, "uRatioHeight");
	ss.h_uRatioWidth = safe_glGetUniformLocation(h, "uRatioWidth");
	ss.h_uXmove = safe_glGetUniformLocation(h, "uYmove");
	ss.h_uYmove = safe_glGetUniformLocation(h, "uXmove");


	// Retrieve handles to vertex attributes
	ss.h_aPosition = safe_glGetAttribLocation(h, "aPosition");
	ss.h_aTexCoord = safe_glGetAttribLocation(h, "aTexCoord");
	ss.h_aColor = safe_glGetAttribLocation(h, "aColor");

	if (!g_Gl2Compatible)
		glBindFragDataLocation(h, 0, "fragColor");
	checkGlErrors();
}

static void initShaders() {
	g_squareShaderState.reset(new SquareShaderState);

	loadSquareShader(*g_squareShaderState);

	g_triangleShaderState.reset(new TriangleShaderState);

	loadTriangleShader(*g_triangleShaderState);
}

static void loadSquareGeometry(const GeometryPX& g) {
	GLfloat pos[12] = {
		-.5, -.5,
		.5,  .5,
		.5,  -.5,

		-.5, -.5,
		-.5, .5,
		.5,  .5
	};

	GLfloat tex[12] = {
		0, 0,
		1, 1,
		1, 0,

		0, 0,
		0, 1,
		1, 1
	};

	glBindBuffer(GL_ARRAY_BUFFER, g.posVbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		12 * sizeof(GLfloat),
		pos,
		GL_STATIC_DRAW);
	checkGlErrors();

	glBindBuffer(GL_ARRAY_BUFFER, g.texVbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		12 * sizeof(GLfloat),
		tex,
		GL_STATIC_DRAW);
	checkGlErrors();
}



static void loadTriangleGeometry(const GeometryPX& g) {
	GLfloat pos[6] = {
		-.25, -.25,
		.25,  .25,
		.5,  -.5
	};

	GLfloat tex[6] = {
		0, 0,
		1, 1,
		1, 0
	};

	GLfloat color[12] = {
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1
	};

	glBindBuffer(GL_ARRAY_BUFFER, g.posVbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		6 * sizeof(GLfloat),
		pos,
		GL_STATIC_DRAW);
	checkGlErrors();

	glBindBuffer(GL_ARRAY_BUFFER, g.texVbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		6 * sizeof(GLfloat),
		tex,
		GL_STATIC_DRAW);
	checkGlErrors();

	glBindBuffer(GL_ARRAY_BUFFER, g.colorVbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		12 * sizeof(GLfloat),
		color,
		GL_STATIC_DRAW);
	checkGlErrors();
}


static void initSquareGeometry() {
	g_square.reset(new GeometryPX());
	loadSquareGeometry(*g_square);

	g_triangle.reset(new GeometryPX());
	loadTriangleGeometry(*g_triangle);
}

static void loadTexture(GLuint texHandle, const char *ppmFilename) {
	int texWidth, texHeight;
	vector<PackedPixel> pixData;

	ppmRead(ppmFilename, texWidth, texHeight, pixData);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, g_Gl2Compatible ? GL_RGB : GL_SRGB, texWidth, texHeight,
		0, GL_RGB, GL_UNSIGNED_BYTE, &pixData[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	checkGlErrors();
}

static void initTextures() {
	g_tex0.reset(new GlTexture());
	g_tex1.reset(new GlTexture());
	g_tex2.reset(new GlTexture());

	loadTexture(*g_tex0, "smiley.ppm");
	loadTexture(*g_tex1, "reachup.ppm");
	loadTexture(*g_tex2, "shield.ppm");
}

