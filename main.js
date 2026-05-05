const canvas = document.getElementById("game");
const ctx = canvas.getContext("2d");

let player = { x: 0, y: 0, speed: 3 };
let camera = { x: 0, y: 0 };

let bullets = [];
let enemies = [];

let keys = {};
let mouse = { x:0, y:0 };

let wave = 1;

document.addEventListener("keydown", e => keys[e.key]=true);
document.addEventListener("keyup", e => keys[e.key]=false);

canvas.addEventListener("mousemove", e => {
    const rect = canvas.getBoundingClientRect();
    mouse.x = e.clientX - rect.left;
    mouse.y = e.clientY - rect.top;
});

canvas.addEventListener("mousedown", () => {
    let wx = mouse.x + camera.x - 400;
    let wy = mouse.y + camera.y - 300;

    let dx = wx - player.x;
    let dy = wy - player.y;

    let len = Math.hypot(dx,dy);
    dx/=len; dy/=len;

    bullets.push({x:player.x,y:player.y,dx,dy,speed:6});
});

function spawnWave() {
    for (let i=0;i<5+wave*2;i++){
        enemies.push({
            x: Math.random()*800,
            y: Math.random()*600,
            speed: 1,
            dead:false
        });
    }
    wave++;
}

function update() {
    if (keys["w"]) player.y -= player.speed;
    if (keys["s"]) player.y += player.speed;
    if (keys["a"]) player.x -= player.speed;
    if (keys["d"]) player.x += player.speed;

    camera.x = player.x;
    camera.y = player.y;

    if (enemies.length === 0)
        spawnWave();

    for (let b of bullets) {
        b.x += b.dx * b.speed;
        b.y += b.dy * b.speed;
    }

    for (let e of enemies) {
        let dx = player.x - e.x;
        let dy = player.y - e.y;
        let len = Math.hypot(dx,dy);
        dx/=len; dy/=len;

        e.x += dx * e.speed;
        e.y += dy * e.speed;
    }

    for (let b of bullets) {
        for (let e of enemies) {
            if (!e.dead && Math.hypot(b.x-e.x,b.y-e.y)<10){
                e.dead=true;
                b.dead=true;
            }
        }
    }

    bullets = bullets.filter(b=>!b.dead);
    enemies = enemies.filter(e=>!e.dead);
}

function draw() {
    ctx.fillStyle="black";
    ctx.fillRect(0,0,800,600);

    // camera transform
    ctx.save();
    ctx.translate(400 - camera.x, 300 - camera.y);

    ctx.fillStyle="green";
    ctx.beginPath();
    ctx.arc(player.x,player.y,10,0,Math.PI*2);
    ctx.fill();

    ctx.fillStyle="red";
    for (let e of enemies){
        ctx.beginPath();
        ctx.arc(e.x,e.y,10,0,Math.PI*2);
        ctx.fill();
    }

    ctx.fillStyle="yellow";
    for (let b of bullets){
        ctx.beginPath();
        ctx.arc(b.x,b.y,4,0,Math.PI*2);
        ctx.fill();
    }

    ctx.restore();

    ctx.fillStyle="white";
    ctx.fillText("Wave: "+wave, 10,20);
}

function loop(){
    update();
    draw();
    requestAnimationFrame(loop);
}

loop();
