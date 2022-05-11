#include "cinder/CameraUi.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Portal {
	vec3 mCenter;
	vec3 mNormal;
	vec2 mDimensions;

	vec3 mBottomLeft;
	vec3 mBottomRight;
	vec3 mTopLeft;
	vec3 mTopRight;

  public:
	Portal() = default;
	Portal( const vec3 &center, const vec3 &normal, const vec2 &dimensions )
		: mCenter{ center }
		, mNormal{ glm::normalize( normal ) }
		, mDimensions{ dimensions }
	{
		getCorners( mTopLeft, mTopRight, mBottomRight, mBottomLeft );
	}
	Portal( const vec3 &topLeft, const vec3 &bottomLeft, const vec3 &bottomRight )
		: mBottomLeft( bottomLeft )
		, mBottomRight( bottomRight )
		, mTopLeft( topLeft )
	{
		const vec3 up = topLeft - bottomLeft;
		const vec3 right = bottomRight - bottomLeft;
		mNormal = glm::normalize( glm::cross( up, right ) );
		mCenter = bottomLeft + 0.5f * ( up + right );
		mDimensions.x = glm::length( right );
		mDimensions.y = glm::length( up );
		mTopRight = mTopLeft + right;
	}

	void getCorners( vec3 &topLeft, vec3 &topRight, vec3 &bottomRight, vec3 &bottomLeft ) const
	{
		const auto up = glm::abs( glm::dot( mNormal, vec3( 0, 1, 0 ) ) ) < 1 ? vec3( 0, 1, 0 ) : vec3( 0, 0, -1 );
		const auto q = glm::quatLookAt( mNormal, up );
		topLeft = q * vec3{ -0.5f * mDimensions.x, 0.5f * mDimensions.y, 0.0f } + mCenter;
		topRight = q * vec3{ 0.5f * mDimensions.x, 0.5f * mDimensions.y, 0.0f } + mCenter;
		bottomRight = q * vec3{ 0.5f * mDimensions.x, -0.5f * mDimensions.y, 0.0f } + mCenter;
		bottomLeft = q * vec3{ -0.5f * mDimensions.x, -0.5f * mDimensions.y, 0.0f } + mCenter;
	}

	float getDistance( const vec3 &point ) const { return glm::dot( point - mCenter, mNormal ); }

	mat4 getViewMatrix( const vec3 &eye ) const { return {}; }

	mat4 getProjectionMatrix( const vec3 &eye, float nearPlane, float farPlane ) const
	{
		vec3 vR = glm::normalize( mBottomRight - mBottomLeft );
		vec3 vU = glm::normalize( mTopLeft - mBottomLeft );
		vec3 vN = -glm::normalize( glm::cross( vR, vU ) );

		vec3 vA = mBottomLeft - eye;
		vec3 vB = mBottomRight - eye;
		vec3 vC = mTopLeft - eye;

		float d = -glm::dot( vA, vN );
		float l = glm::dot( vR, vA ) * nearPlane / d;
		float r = glm::dot( vR, vB ) * nearPlane / d;
		float b = glm::dot( vU, vA ) * nearPlane / d;
		float t = glm::dot( vU, vC ) * nearPlane / d;

		mat3 perpendicular{ vR, vU, vN };

		mat4 projection = glm::frustum( l, r, b, t, nearPlane, farPlane );
		projection *= glm::transpose( mat4( perpendicular ) );
		projection *= glm::translate( -eye );

		return projection;
	}

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

	void draw( const vec3 &point, const Color &color = Color::white() ) const
	{
		vec3 n = point - getDistance( point ) * mNormal;

		gl::ScopedColor scpColor( color );

		gl::begin( GL_LINES );
		gl::vertex( point );
		gl::vertex( mTopLeft );
		gl::vertex( point );
		gl::vertex( mTopRight );
		gl::vertex( point );
		gl::vertex( mBottomRight );
		gl::vertex( point );
		gl::vertex( mBottomLeft );
		gl::vertex( point );
		gl::vertex( n );
		gl::end();
	}

	void draw( const gl::Texture2dRef &texture ) const
	{
		gl::ScopedColor       scpColor( 1, 1, 1 );
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
	Portal mLeft{ { -5, 0, 0 }, { 1, 0, 0 }, { 9.75f, 9.75f } };
	Portal mFront{ { 0, 0, -5 }, { 0, 0, 1 }, { 9.75f, 9.75f } };
	Portal mRight{ { 5, 0, 0 }, { -1, 0, 0 }, { 9.75f, 9.75f } };
};

void PortalsApp::setup()
{
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
}

void PortalsApp::draw()
{
	gl::ScopedDepth scpDepth( true );

	gl::clear( Color::hex( 0x2d2d2d ) );
	gl::color( 1, 1, 1 );

	gl::setMatrices( mCamera );

	//
	mLeft.draw( Color( 1, 0, 0 ) );
	mFront.draw( Color( 0, 1, 0 ) );
	mRight.draw( Color( 0, 0, 1 ) );

	mLeft.draw( mEye, Color( 1, 0.7f, 0.7f ) );
	mFront.draw( mEye, Color( 0.7f, 1, 0.7f ) );
	mRight.draw( mEye, Color( 0.7f, 0.7f, 1 ) );

	//
	{
		gl::ScopedGlslProg scpGlsl( gl::getStockShader( gl::ShaderDef().lambert().color() ) );
		gl::draw( mBox );
		gl::draw( mSphere );
		gl::draw( mCapsule );
		gl::draw( mTeapot );
	}

	if( mFbo ) {
		gl::ScopedFramebuffer scpFbo( mFbo );
		gl::ScopedViewport    scpViewport( mFbo->getSize() );
		gl::ScopedMatrices    scpMatrices;
		gl::setModelMatrix( mat4() );
		gl::setViewMatrix( mLeft.getViewMatrix( mEye ) );
		gl::setProjectionMatrix( mLeft.getProjectionMatrix( mEye, 0.5f, 50.0f ) );

		gl::ScopedPolygonMode scpPoly( GL_LINE );
		gl::ScopedFaceCulling scpCull( true );
		gl::clear( ColorA( 0, 0, 0, 0 ) );

		gl::ScopedColor scpColor( 0.5f, 0.5f, 0.5f );
		gl::draw( mBox );
		gl::draw( mSphere );
		gl::draw( mCapsule );
		gl::draw( mTeapot );
	}
	if( mFbo ) {
		gl::ScopedDepth        scpDepthDisable( false );
		gl::ScopedBlendPremult scpBlend;
		mLeft.draw( mFbo->getColorTexture() );
	}

	if( mFbo ) {
		gl::ScopedFramebuffer scpFbo( mFbo );
		gl::ScopedViewport    scpViewport( mFbo->getSize() );
		gl::ScopedMatrices    scpMatrices;
		gl::setModelMatrix( mat4() );
		gl::setViewMatrix( mFront.getViewMatrix( mEye ) );
		gl::setProjectionMatrix( mFront.getProjectionMatrix( mEye, 0.5f, 50.0f ) );

		gl::ScopedPolygonMode scpPoly( GL_LINE );
		gl::ScopedFaceCulling scpCull( true );
		gl::clear( ColorA( 0, 0, 0, 0 ) );

		gl::ScopedColor scpColor( 0.5f, 0.5f, 0.5f );
		gl::draw( mBox );
		gl::draw( mSphere );
		gl::draw( mCapsule );
		gl::draw( mTeapot );
	}
	if( mFbo ) {
		gl::ScopedDepth        scpDepthDisable( false );
		gl::ScopedBlendPremult scpBlend;
		mFront.draw( mFbo->getColorTexture() );
	}

	if( mFbo ) {
		gl::ScopedFramebuffer scpFbo( mFbo );
		gl::ScopedViewport    scpViewport( mFbo->getSize() );
		gl::ScopedMatrices    scpMatrices;
		gl::setModelMatrix( mat4() );
		gl::setViewMatrix( mRight.getViewMatrix( mEye ) );
		gl::setProjectionMatrix( mRight.getProjectionMatrix( mEye, 0.5f, 50.0f ) );

		gl::ScopedPolygonMode scpPoly( GL_LINE );
		gl::ScopedFaceCulling scpCull( true );
		gl::clear( ColorA( 0, 0, 0, 0 ) );

		gl::ScopedColor scpColor( 0.5f, 0.5f, 0.5f );
		gl::draw( mBox );
		gl::draw( mSphere );
		gl::draw( mCapsule );
		gl::draw( mTeapot );
	}
	if( mFbo ) {
		gl::ScopedDepth        scpDepthDisable( false );
		gl::ScopedBlendPremult scpBlend;
		mRight.draw( mFbo->getColorTexture() );
	}
}

void PortalsApp::keyDown( KeyEvent event )
{
	mUseCamera = true;
}

void PortalsApp::keyUp( KeyEvent event )
{
	mUseCamera = false;
}

CINDER_APP( PortalsApp, RendererGl( RendererGl::Options().msaa( 16 ) ), &PortalsApp::prepare )
