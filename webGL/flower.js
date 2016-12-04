
// scene size
var WIDTH = window.innerWidth;
var HEIGHT = window.innerHeight;

// camera
var VIEW_ANGLE = 45;
var ASPECT = WIDTH / HEIGHT;
var NEAR = 1;
var FAR = 500;

var camera, scene, renderer;

var cameraControls;

var verticalMirror, groundMirror;

var bloom = -10;

var pink_lathes = [];
var blue_lathes = []; 
var orange_lathes = []; 
var red_lathes = []; 

// color 0
var pink = [0xed5e5e, 0xef7777, 0xf48686, 0xfc9797 , 0xffadad, 0xffc1c1 , 0xffd1d1, 0xffe2e2, 0xfff2f2,0xfff7f7 ,0xffffff];
// color 1
var blue = [0x00AEFF, 0x00CDFF, 0x25D4FF, 0x43DAFF, 0x5EDFFF, 0x7DE5FF, 0x94EAFF, 0xA8EEFF, 0xC3F1FC, 0xD9F8FF, 0xFFFFFF];
var orange = [0xFF8000, 0xFF8B16, 0xFF9326, 0xFF9C38, 0xFFA850, 0xFFB265, 0xFFBF80, 0xFFCC9A, 0xFFDBB7, 0xFEEEDE, 0xffffff];
var red = [0xFF0000, 0xFF1A1A, 0xFF2C2C, 0xFE4A4A, 0xFF6161, 0xFE7575, 0xFE8C8C, 0xFFA1A1, 0xFEB7B7, 0xFED4D4, 0xffffff];

function init() {

	// renderer
	renderer = new THREE.WebGLRenderer();
	renderer.setPixelRatio( window.devicePixelRatio );
	renderer.setSize( WIDTH, HEIGHT );

	// scene
	scene = new THREE.Scene();

	// camera
	camera = new THREE.PerspectiveCamera(VIEW_ANGLE, ASPECT, NEAR, FAR);
	camera.position.set( 0, 75, 160 );

	cameraControls = new THREE.OrbitControls(camera, renderer.domElement);
	cameraControls.target.set( 0, 40, 0);
	cameraControls.maxDistance = 400;
	cameraControls.minDistance = 10;
	cameraControls.update();

	var container = document.getElementById( 'container' );
	container.appendChild( renderer.domElement );


}


function fillScene() {

	var planeGeo = new THREE.PlaneBufferGeometry( 100.1, 100.1 );

	// MIRROR planes
	groundMirror = new THREE.Mirror( renderer, camera, { clipBias: 0.003, textureWidth: WIDTH, textureHeight: HEIGHT, color: 0x777777 } );

	var mirrorMesh = new THREE.Mesh( planeGeo, groundMirror.material );
	mirrorMesh.add( groundMirror );
	mirrorMesh.rotateX( - Math.PI / 2 );
	scene.add( mirrorMesh );

	// vert mirror
	verticalMirror = new THREE.Mirror( renderer, camera, { clipBias: 0.003, textureWidth: WIDTH, textureHeight: HEIGHT, color:0x889999 } );

	var verticalMirrorMesh = new THREE.Mesh( new THREE.PlaneBufferGeometry( 60, 60 ), verticalMirror.material );
	verticalMirrorMesh.add( verticalMirror );
	verticalMirrorMesh.position.y = 35;
	verticalMirrorMesh.position.z = -45;
	scene.add( verticalMirrorMesh );

	// walls
	var planeTop = new THREE.Mesh( planeGeo, new THREE.MeshPhongMaterial( { color: 0xffffff } ) );
	planeTop.position.y = 100;
	planeTop.rotateX( Math.PI / 2 );
	scene.add( planeTop );

	var planeBack = new THREE.Mesh( planeGeo, new THREE.MeshPhongMaterial( { color: 0xffffff } ) );
	planeBack.position.z = -50;
	planeBack.position.y = 50;
	scene.add( planeBack );

	var planeFront = new THREE.Mesh( planeGeo, new THREE.MeshPhongMaterial( { color: 0xffffff } ) );
	planeFront.position.z = 50;
	planeFront.position.y = 50;
	planeFront.rotateY( Math.PI );
	scene.add( planeFront );

	var planeRight = new THREE.Mesh( planeGeo, new THREE.MeshPhongMaterial( { color: 0xffffff } ) );
	planeRight.position.x = 50;
	planeRight.position.y = 50;
	planeRight.rotateY( - Math.PI / 2 );
	scene.add( planeRight );

	var planeLeft = new THREE.Mesh( planeGeo, new THREE.MeshPhongMaterial( { color: 0xffffff} ) );
	planeLeft.position.x = -50;
	planeLeft.position.y = 50;
	planeLeft.rotateY( Math.PI / 2 );
	scene.add( planeLeft );

	// lights
	var mainLight = new THREE.PointLight( 0xcccccc, 1.5, 250 );
	mainLight.position.y = 60;
	scene.add( mainLight );

	//drawFlower(1, 1, blue);
}

function drawFlower(x, y, c) { 
	// lathe 1

	if (c == red) {
		for (var i = 0; i < red_lathes.length; i++) 
			scene.remove(red_lathes[i]);
		for (var j = 0; j < 11; j++) {
			var points = [];
			for (var i = 0; i < 10; i++) 
				points.push(new THREE.Vector2(Math.sin(i * 0.2) * (10 - j) + 5 + bloom, (i + 1) * 2));

			var geometry = new THREE.LatheGeometry( points );
			var material = new THREE.MeshBasicMaterial( { color: red[j]} );
			var lathe = new THREE.Mesh( geometry, material );
			lathe.position = new THREE.Vector3(x, y, 0);
			red_lathes.push(lathe);
			scene.add( lathe );
			
		}
		return;
	}
	if (c == blue) {
		for (var i = 0; i < blue_lathes.length; i++) 
			scene.remove(blue_lathes[i]);
		// for each layer
		for (var j = 0; j < 1; j++) {
			var points = [];
			for (var i = 0; i < 10; i++) 
				points.push(new THREE.Vector2(Math.sin(i * 0.2) * (10 - j) + 5 + bloom, (i + 1) * 2));
			start = 0 ;
			for (var petal = 0; petal < 8; petal++) {
				var geometry = new THREE.LatheGeometry( points, 12,  start, Math.PI/4 - Math.PI/15);
				var material = new THREE.MeshBasicMaterial( { color: blue[petal]} );
				var lathe = new THREE.Mesh( geometry, material );
				lathe.position.x = 0;
				lathe.position.y = 0;
				blue_lathes.push(lathe);
				scene.add( lathe );
				start += Math.PI/4;
			}
			
			
		}
		return;
	}
	if (c == pink) {
		for (var i = 0; i < pink_lathes.length; i++) 
			scene.remove(pink_lathes[i]);
		// for each layer
		for (var j = 0; j < 1; j++) {
			var points = [];
			for (var i = 0; i < 10; i++) 
				points.push(new THREE.Vector2(Math.sin(i * 0.2) * (10 - j) + 5 + bloom, (i + 1) * 2));
			start = 0 ;
			for (var petal = 0; petal < 8; petal++) {
				var geometry = new THREE.LatheGeometry( points, 12,  start, Math.PI/4 - Math.PI/15);
				var material = new THREE.MeshBasicMaterial( { color: pink[j]} );
				var lathe = new THREE.Mesh( geometry, material );
				lathe.position.x = -10;
				lathe.position.y = -10;
				pink_lathes.push(lathe);
				scene.add( lathe );
				start += Math.PI/4;
			}
			
			
		}
		return;
	}
	if (c == orange) {
		for (var i = 0; i < orange_lathes.length; i++) 
			scene.remove(orange_lathes[i]);
		// for each layer
		for (var j = 0; j < 1; j++) {
			var points = [];
			for (var i = 0; i < 10; i++) 
				points.push(new THREE.Vector2(Math.sin(i * 0.2) * (10 - j) + 5 + bloom, (i + 1) * 2));
			start = 0 ;
			for (var petal = 0; petal < 8; petal++) {
				var geometry = new THREE.LatheGeometry( points, 12,  start, Math.PI/4 - Math.PI/15);
				var material = new THREE.MeshBasicMaterial( { color: orange[j]} );
				var lathe = new THREE.Mesh( geometry, material );
				lathe.position.x = 0;
				lathe.position.y = 0;
				orange_lathes.push(lathe);
				scene.add( lathe );
				start += Math.PI/4;
			}
			
			
		}
		return;
	}

}

function render() {
	// render (update) the mirrors
	drawFlower(1, 1, blue);
	//drawFlower(1, 1, orange);
	//drawFlower(1, 1, pink);
	groundMirror.renderWithMirror( verticalMirror );
	verticalMirror.renderWithMirror( groundMirror );
	renderer.render(scene, camera);

}

var currentlyPressedKeys = {};

function handleKeyDown(event) {
    currentlyPressedKeys[event.keyCode] = true;
}

function handleKeyUp(event) {
	currentlyPressedKeys[event.keyCode] = false;
}


function handleKeys() {
    if (currentlyPressedKeys[85]) {
    	if (bloom != 0)
    	bloom += .5;
    }
    if (currentlyPressedKeys[74]) {
    	if (bloom != -10)
    		bloom -= .5;
    }
 }

function animate() {
	requestAnimationFrame( animate );
	handleKeys();
	render();
}

function update() {
	requestAnimationFrame( update );
	var timer = Date.now() * 0.01;
	cameraControls.update();
	render();
}

document.onkeydown = handleKeyDown;
document.onkeyup = handleKeyUp;
init();
fillScene();
update();
animate();