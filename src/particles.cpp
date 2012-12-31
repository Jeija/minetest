/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "particles.h"
#include "constants.h"
#include "debug.h"
#include "main.h" // For g_profiler and g_settings
#include "settings.h"
#include "tile.h"
#include "gamedef.h"
#include <stdlib.h>
#include "util/numeric.h"

Particle::Particle(
	IGameDef *gamedef,
	scene::ISceneManager* smgr,
	LocalPlayer *player,
	s32 id,
	v3f pos,
	v3f velocity,
	v3f acceleration,
	float expirationtime,
	float size
):
	scene::ISceneNode(smgr->getRootSceneNode(), smgr, id)
{
	// Misc
	m_gamedef = gamedef;

	// Texture
	m_material.setFlag(video::EMF_LIGHTING, false);
	m_material.setFlag(video::EMF_BACK_FACE_CULLING, false);
	m_material.setFlag(video::EMF_BILINEAR_FILTER, false);
	m_material.setFlag(video::EMF_FOG_ENABLE, true);
	AtlasPointer ap = m_gamedef->tsrc()->getTexture("default_dirt.png");
	m_material.setTexture(0, ap.atlas);
	tex_x0 = ap.x0();
	tex_x1 = ap.x1();
	tex_y0 = ap.y0();
	tex_y1 = ap.y1();


	// Particle related
	m_pos = pos;
	m_velocity = velocity;
	m_acceleration = acceleration;
	m_expiration = expirationtime;
	m_time = 0;
	m_player = player;
	m_size = size;

	// Irrlicht stuff (TODO)
	m_box = core::aabbox3d<f32>(-size, -size, -size, size, size, size);
	std::cout<<"new particle"<<std::endl;
	this->setAutomaticCulling(scene::EAC_OFF);
}

Particle::~Particle()
{
}

void Particle::OnRegisterSceneNode()
{
	//SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);
	SceneManager->registerNodeForRendering(this, scene::ESNRP_SOLID);

	ISceneNode::OnRegisterSceneNode();
}

void Particle::render()
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	driver->setMaterial(m_material);
	driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
	video::SColor c(255, 255, 255, 255);

	video::S3DVertex vertices[4] =
	{
		video::S3DVertex(-m_size/2,-m_size/2,0, 0,0,0, c, 
		tex_x0, tex_y1),
		video::S3DVertex(m_size/2,-m_size/2,0, 0,0,0, c, 
		tex_x1, tex_y1),
		video::S3DVertex(m_size/2,m_size/2,0, 0,0,0, c, 
		tex_x1, tex_y0),
		video::S3DVertex(-m_size/2,m_size/2,0, 0,0,0, c, 
		tex_x0, tex_y0),
	};

	for(u16 i=0; i<4; i++)
	{
		vertices[i].Pos.rotateYZBy(m_player->getPitch());
		vertices[i].Pos.rotateXZBy(m_player->getYaw());
		vertices[i].Pos += m_pos*BS;
            m_box.addInternalPoint(vertices[i].Pos);
	}

	u16 indices[] = {0,1,2, 2,3,0};
	driver->drawVertexPrimitiveList(vertices, 4, indices, 2,
			video::EVT_STANDARD, scene::EPT_TRIANGLES, video::EIT_16BIT);
}

void Particle::step(float dtime)
{
	m_time += dtime;
	m_velocity += m_acceleration * dtime;
	m_pos += m_velocity * dtime;
}

// To be changed TODO!
Particle *all_particles[10000] = {NULL};

void allparticles_step (float dtime)
{
	for(u16 i = 0; i< 10000; i++)
	{
		if (all_particles[i] != NULL)
			all_particles[i]->step(dtime);
	}
}

void addDiggingParticles(IGameDef* gamedef, scene::ISceneManager* smgr, LocalPlayer *player, v3s16 pos)
{
	for (u16 j = 0; j < 10; j++)
	{
		v3f velocity(rand()%100/100.-0.5, 2, rand()%100/100.-0.5);
		v3f acceleration(0,-9,0);
		v3f particlepos = v3f(
			(f32)pos.X+rand()%100/200.-0.25,
			(f32)pos.Y+rand()%100/200.-0.25,
			(f32)pos.Z+rand()%100/200.-0.25
		);
		Particle *particle = new Particle(
			gamedef,
			smgr,
			player,
			0,
			particlepos,
			velocity,
			acceleration,
			10,
			BS/(rand()%10+4));

		for (u16 i = 0; i< 10000; i++)
		{
			if (all_particles[i] == NULL) 
			{
				all_particles[i] = particle;
				break;
			}
		}
	}
}
