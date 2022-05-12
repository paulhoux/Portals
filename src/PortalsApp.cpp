#include "cinder/CameraUi.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Portal {
	vec3 mBottomLeft;
	vec3 mBottomRight;
	vec3 mTopLeft;
	vec3 mTopRight;

  public:
	Portal() = default;

	//! Constructs a Portal by defining 3 of its 4 corners (the 4th corner is implied).
	Portal( const vec3 &bottomLeft, const vec3 &bottomRight, const vec3 &topLeft )
		: mBottomLeft( bottomLeft )
		, mBottomRight( bottomRight )
		, mTopLeft( topLeft )
	{
		mTopRight = mTopLeft + mBottomRight - mBottomLeft;
	}

	//! Returns the distance from the \a eye to the nearest point on the Portal.
	float getDistance( const vec3 &eye ) const
	{
		vec3 vR = glm::normalize( mBottomRight - mBottomLeft );
		vec3 vU = glm::normalize( mTopLeft - mBottomLeft );
		vec3 vN = glm::normalize( glm::cross( vR, vU ) );
		vec3 vA = mBottomLeft - eye;
		return -glm::dot( vA, vN );
	}

	//! Calculates and returns the projection matrix. Your view matrix should be set to identity.
	mat4 getProjectionMatrix( const vec3 &eye, float nearPlane, float farPlane ) const
	{
		vec3 vR = glm::normalize( mBottomRight - mBottomLeft ); // right
		vec3 vU = glm::normalize( mTopLeft - mBottomLeft );     // up
		vec3 vN = glm::normalize( glm::cross( vR, vU ) );       // normal

		vec3 vA = mBottomLeft - eye;
		vec3 vB = mBottomRight - eye;
		vec3 vC = mTopLeft - eye;

		float d = -glm::dot( vA, vN ); // distance
		float s = nearPlane / d;       // scaling

		float l = glm::dot( vR, vA ) * s; // frustum left
		float r = glm::dot( vR, vB ) * s; // frustum right
		float b = glm::dot( vU, vA ) * s; // frustum bottom
		float t = glm::dot( vU, vC ) * s; // frustum top
		mat4  projection = glm::frustum( l, r, b, t, nearPlane, farPlane );

		mat3 perpendicular{ vR, vU, vN }; // projection plane orientation
		projection *= mat4( glm::transpose( perpendicular ) );

		projection *= glm::translate( -eye ); // view point offset

		return projection;
	}

	//! Draws the Portal rectangle.
	void draw( const Color &color = Color::white() ) const
	{
		gl::ScopedColor scpColor( color );

		gl::begin( GL_LINE_STRIP );
		gl::vertex( mTopLeft );
		gl::vertex( mTopRight );
		gl::vertex( mBottomRight );
		gl::vertex( mBottomLeft );
		gl::vertex( mTopLeft );
		gl::end();
	}

	//! Draws the Portal's frustum.
	void draw( const vec3 &eye, const Color &color = Color::white() ) const
	{
		vec3 vR = glm::normalize( mBottomRight - mBottomLeft );
		vec3 vU = glm::normalize( mTopLeft - mBottomLeft );
		vec3 vN = glm::normalize( glm::cross( vR, vU ) );
		vec3 vA = mBottomLeft - eye;

		vec3 nearest = eye + glm::dot( vA, vN ) * vN;

		gl::ScopedColor scpColor( color );

		gl::begin( GL_LINES );
		gl::vertex( eye );
		gl::vertex( mTopLeft );
		gl::vertex( eye );
		gl::vertex( mTopRight );
		gl::vertex( eye );
		gl::vertex( mBottomRight );
		gl::vertex( eye );
		gl::vertex( mBottomLeft );
		gl::vertex( eye );
		gl::vertex( nearest );
		gl::end();
	}

	//! Draws the texture inside the Portal rectangle.
	void draw( const gl::Texture2dRef &texture ) const
	{
		gl::ScopedTextureBind scpTex( texture, 0 );

		auto glsl = gl::getStockShader( gl::ShaderDef().color().texture( texture ) );

		gl::ScopedGlslProg glslScp( glsl );
		glsl->uniform( "uTex0", 0 );

		gl::setDefaultShaderVars();

		gl::begin( GL_TRIANGLE_STRIP );
		gl::texCoord( 0, 1 );
		gl::texCoord( 1, 1 );
		gl::texCoord( 0, 0 );
		gl::texCoord( 1, 0 );
		gl::vertex( mTopLeft );
		gl::vertex( mTopRight );
		gl::vertex( mBottomLeft );
		gl::vertex( mBottomRight );
		gl::end();
	}
};

class PortalsApp : public App {
  public:
	static void prepare( Settings *settings ) { settings->setWindowSize( 1600, 900 ); }

	void setup() override;
	void update() override;
	void draw() override;

	void keyDown( KeyEvent event ) override;
	void keyUp( KeyEvent event ) override;

	void renderPortalView( const Portal &portal ) const;
	void renderPortalTexture( const Portal &portal, const Rectf &bounds, const Color &color ) const;

  private:
	CameraPersp    mCamera;
	CameraUi       mCameraUi;
	gl::VboMeshRef mBox;
	gl::VboMeshRef mSphere;
	gl::VboMeshRef mCapsule;
	gl::VboMeshRef mTeapot;
	gl::FboRef     mFbo;
	bool           mUseCamera = false;

	vec3   mEye{ -2, 0, 5 };
	Portal mLeft{ { -5, -5, 5.25f }, { -5, -5, -4.75f }, { -5, 5, 5.25f } };
	Portal mFront{ { -5, -5, -5 }, { 5, -5, -5 }, { -5, 5, -5 } };
	Portal mRight{ { 5, -5, -4.75f }, { 5, -5, 5.25f }, { 5, 5, -4.75f } };
};

void PortalsApp::setup()
{
	mCamera.lookAt( { -8, 10, 34 }, { 0.75f, -3.75f, 0 } );

	mCameraUi.setCamera( &mCamera );
	mCameraUi.connect( getWindow() );

	mBox = gl::VboMesh::create( geom::Cube().size( 5, 5, 5 ) >> geom::Rotate( glm::radians( 45.0f ), vec3( 0, 1, 0 ) ) >> geom::Translate( 5, 0, -10 ) );
	mSphere = gl::VboMesh::create( geom::Icosphere().subdivisions( 2 ) >> geom::Scale( 2.5f ) >> geom::Translate( -5, 0, -10 ) );
	mCapsule = gl::VboMesh::create( geom::Capsule().subdivisionsAxis( 30 ).subdivisionsHeight( 10 ).length( 4 ).radius( 1 ) >> geom::Translate( -10, 0, 0 ) );
	mTeapot = gl::VboMesh::create( geom::Teapot().subdivisions( 6 ) >> geom::Scale( 5.0f ) >> geom::Rotate( glm::radians( 90.0f ), vec3( 0, 1, 0 ) ) >> geom::Translate( 10, -2, 0 ) );

	mFbo = gl::Fbo::create( 1024, 1024 );
}

void PortalsApp::update()
{
	if( mUseCamera ) {
		mEye = mCamera.getEyePoint();
	}
	else {
		auto t = float( getElapsedSeconds() * 0.25 );
		mEye.x = 4.0f * glm::sin( t );
		mEye.y = 4.0f * glm::cos( t * 1.7f );
		mEye.z = 4.0f * glm::sin( t * 1.1f );
	}

	auto eye = mCamera.getEyePoint();
	auto lookat = mCamera.getPivotPoint();
}

void PortalsApp::draw()
{
	// Prepare to render 3D.
	gl::ScopedDepth scpDepth( true );

	gl::clear( Color::hex( 0x2d2d2d ) );
	gl::color( 1, 1, 1 );

	gl::setMatrices( mCamera );

	// Render Portal edges and frustum.
	mLeft.draw( Color( 1, 0, 0 ) );
	mFront.draw( Color( 0, 1, 0 ) );
	mRight.draw( Color( 0, 0, 1 ) );

	mLeft.draw( mEye, Color( 1, 0.7f, 0.7f ) );
	mFront.draw( mEye, Color( 0.7f, 1, 0.7f ) );
	mRight.draw( mEye, Color( 0.7f, 0.7f, 1 ) );

	// Render 3D scene.
	{
		gl::ScopedGlslProg scpGlsl( gl::getStockShader( gl::ShaderDef().lambert().color() ) );
		gl::draw( mBox );
		gl::draw( mSphere );
		gl::draw( mCapsule );
		gl::draw( mTeapot );
	}

	//
	auto x = 0.5f * float( getWindowWidth() - 768 );
	auto y = float( getWindowHeight() - 256 );

	renderPortalView( mLeft );
	renderPortalTexture( mLeft, Rectf( x, y, x + 256, y + 256 ), Color( 1, 0, 0 ) );

	renderPortalView( mFront );
	renderPortalTexture( mFront, Rectf( x + 256, y, x + 512, y + 256 ), Color( 0, 1, 0 ) );

	renderPortalView( mRight );
	renderPortalTexture( mRight, Rectf( x + 512, y, x + 768, y + 256 ), Color( 0, 0, 1 ) );
}

void PortalsApp::keyDown( KeyEvent event )
{
	mUseCamera = true;
}

void PortalsApp::keyUp( KeyEvent event )
{
	mUseCamera = false;
}

void PortalsApp::renderPortalView( const Portal &portal ) const
{
	if( mFbo ) {
		gl::ScopedFramebuffer scpFbo( mFbo );
		gl::ScopedViewport    scpViewport( mFbo->getSize() );
		gl::ScopedMatrices    scpMatrices;
		gl::setModelMatrix( mat4() );
		gl::setViewMatrix( mat4() );
		gl::setProjectionMatrix( portal.getProjectionMatrix( mEye, 0.5f, 500.0f ) );

		gl::ScopedPolygonMode scpPoly( GL_LINE );
		gl::ScopedFaceCulling scpCull( true );
		gl::clear( ColorA( 0, 0, 0, 0 ) );

		gl::ScopedColor scpColor( 0.5f, 0.5f, 0.5f );
		gl::draw( mBox );
		gl::draw( mSphere );
		gl::draw( mCapsule );
		gl::draw( mTeapot );
	}
}

void PortalsApp::renderPortalTexture( const Portal &portal, const Rectf &bounds, const Color &color ) const
{
	if( mFbo ) {
		gl::ScopedDepth        scpDepthDisable( false );
		gl::ScopedBlendPremult scpBlend;
		portal.draw( mFbo->getColorTexture() );

		gl::ScopedMatrices scpMatrices;
		gl::setMatricesWindow( getWindowSize() );
		gl::draw( mFbo->getColorTexture(), bounds );

		gl::ScopedColor scpColor( color );
		gl::drawStrokedRect( bounds.inflated( vec2( -0.5f ) ), 1 );
	}
}

CINDER_APP( PortalsApp, RendererGl( RendererGl::Options().msaa( 16 ) ), &PortalsApp::prepare )
