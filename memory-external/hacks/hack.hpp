#include <thread>
#include <cmath>
#include "reader.hpp"
#include "../classes/render_dx11.hpp"
#include "../classes/config.hpp"
#include "../classes/globals.hpp"

namespace hack {
	std::vector<std::pair<std::string, std::string>> boneConnections = {
						{"neck_0", "spine_1"},
						{"spine_1", "spine_2"},
						{"spine_2", "pelvis"},
						{"spine_1", "arm_upper_L"},
						{"arm_upper_L", "arm_lower_L"},
						{"arm_lower_L", "hand_L"},
						{"spine_1", "arm_upper_R"},
						{"arm_upper_R", "arm_lower_R"},
						{"arm_lower_R", "hand_R"},
						{"pelvis", "leg_upper_L"},
						{"leg_upper_L", "leg_lower_L"},
						{"leg_lower_L", "ankle_L"},
						{"pelvis", "leg_upper_R"},
						{"leg_upper_R", "leg_lower_R"},
						{"leg_lower_R", "ankle_R"}
	};

	void loop(ImDrawList* drawList) {

		std::lock_guard<std::mutex> lock(reader_mutex);

		if (g_game.isC4Planted)
		{
			Vector3 c4Origin = g_game.c4.get_origin();
			const Vector3 c4ScreenPos = g_game.world_to_screen(&c4Origin);

			if (c4ScreenPos.z >= 0.01f) {
				float c4Distance = g_game.localOrigin.calculate_distance(c4Origin);
				float c4RoundedDistance = std::round(c4Distance / 500.f);

				float height = 10 - c4RoundedDistance;
				float width = height * 1.4f;

				render::DrawFilledBox(
					drawList,
					c4ScreenPos.x - (width / 2),
					c4ScreenPos.y - (height / 2),
					width,
					height,
					ImColor(config::esp_box_color_enemy.r, config::esp_box_color_enemy.g, config::esp_box_color_enemy.b)
				);

				render::RenderText(
					drawList,
					c4ScreenPos.x + (width / 2 + 5),
					c4ScreenPos.y,
					"C4",
					ImColor(config::esp_name_color.r, config::esp_name_color.g, config::esp_name_color.b),
					12.0f
				);
			}
		}

		int playerIndex = 0;
		uintptr_t list_entry;

		/**
		* Loop through all the players in the entity list
		*
		* (This could have been done by getting a entity list count from the engine, but I'm too lazy to do that)
		**/
		for (auto player = g_game.players.begin(); player < g_game.players.end(); player++) {
			const Vector3 screenPos = g_game.world_to_screen(&player->origin);
			const Vector3 screenHead = g_game.world_to_screen(&player->head);

			if (
				screenPos.z < 0.01f || 
				!utils.is_in_bounds(screenPos, g_game.game_bounds.right, g_game.game_bounds.bottom)
				)
				continue;

			const float height = screenPos.y - screenHead.y;
			const float width = height / 2.4f;

			float distance = g_game.localOrigin.calculate_distance(player->origin);
			int roundedDistance = std::round(distance / 10.f);

			if (config::show_head_tracker) {
				render::DrawCircle(
					drawList,
					player->bones.bonePositions["head"].x,
					player->bones.bonePositions["head"].y - width / 12,
					width / 5,
					(g_game.localTeam == player->team ? ImColor(config::esp_skeleton_color_team.r, config::esp_skeleton_color_team.g, config::esp_skeleton_color_team.b) : ImColor(config::esp_skeleton_color_enemy.r, config::esp_skeleton_color_enemy.g, config::esp_skeleton_color_enemy.b))
				);
			}

			if (config::show_skeleton_esp) {
				for (const auto& connection : boneConnections) {
					const std::string& boneFrom = connection.first;
					const std::string& boneTo = connection.second;

					render::DrawLine(
						drawList,
						player->bones.bonePositions[boneFrom].x, player->bones.bonePositions[boneFrom].y,
						player->bones.bonePositions[boneTo].x, player->bones.bonePositions[boneTo].y,
						g_game.localTeam == player->team ? ImColor(config::esp_skeleton_color_team.r, config::esp_skeleton_color_team.g, config::esp_skeleton_color_team.b) : ImColor(config::esp_skeleton_color_enemy.r, config::esp_skeleton_color_enemy.g, config::esp_skeleton_color_enemy.b)
					);
				}
			}

			if (config::show_box_esp)
			{
				render::DrawBorderBox(
					drawList,
					screenHead.x - width / 2,
					screenHead.y,
					width,
					height,
					(g_game.localTeam == player->team ? ImColor(config::esp_box_color_team.r, config::esp_box_color_team.g, config::esp_box_color_team.b) : ImColor(config::esp_box_color_enemy.r, config::esp_box_color_enemy.g, config::esp_box_color_enemy.b))
				);
			}

			render::DrawBorderBox(
				drawList,
				screenHead.x - (width / 2 + 10),
				screenHead.y + (height * (100 - player->armor) / 100),
				2,
				height - (height * (100 - player->armor) / 100),
				ImColor(0, 185, 255)
			);

			render::DrawBorderBox(
				drawList,
				screenHead.x - (width / 2 + 5),
				screenHead.y + (height * (100 - player->health) / 100),
				2,
				height - (height * (100 - player->health) / 100),
				ImColor(
					(255 - player->health),
					(55 + player->health * 2),
					75
				)
			);

			render::RenderText(
				drawList,
				screenHead.x + (width / 2 + 5),
				screenHead.y,
				player->name.c_str(),
				ImColor(config::esp_name_color.r, config::esp_name_color.g, config::esp_name_color.b),
				12.0f
			);

			/**
			* I know is not the best way but a simple way to not saturate the screen with a ton of information
			*/
			if (roundedDistance > config::flag_render_distance)
				continue;

			render::RenderText(
				drawList,
				screenHead.x + (width / 2 + 5),
				screenHead.y + 12,
				(std::to_string(player->health) + "hp").c_str(),
				ImColor(
					(255 - player->health),
					(55 + player->health * 2),
					75
				),
				12.0f
			);

			render::RenderText(
				drawList,
				screenHead.x + (width / 2 + 5),
				screenHead.y + 24,
				(std::to_string(player->armor) + "armor").c_str(),
				ImColor(
					(255 - player->armor),
					(55 + player->armor * 2),
					75
				),
				12.0f
			);

			if (config::show_extra_flags)
			{
				render::RenderText(
					drawList,
					screenHead.x + (width / 2 + 5),
					screenHead.y + 36,
					player->weapon.c_str(),
					ImColor(config::esp_distance_color.r, config::esp_distance_color.g, config::esp_distance_color.b),
					12.0f
				);

				render::RenderText(
					drawList,
					screenHead.x + (width / 2 + 5),
					screenHead.y + 48,
					(std::to_string(roundedDistance) + "m away").c_str(),
					ImColor(config::esp_distance_color.r, config::esp_distance_color.g, config::esp_distance_color.b),
					12.0f
				);

				render::RenderText(
					drawList,
					screenHead.x + (width / 2 + 5),
					screenHead.y + 60,
					("$" + std::to_string(player->money)).c_str(),
					ImColor(0, 125, 0),
					12.0f
				);

				if (player->flashAlpha > 100)
				{
					render::RenderText(
						drawList,
						screenHead.x + (width / 2 + 5),
						screenHead.y + 72,
						"Player is flashed",
						ImColor(config::esp_distance_color.r, config::esp_distance_color.g, config::esp_distance_color.b),
						12.0f
					);
				}

				if (player->is_defusing)
				{
					const std::string defuText = "Player is defusing";
					render::RenderText(
						drawList,
						screenHead.x + (width / 2 + 5),
						screenHead.y + 72,
						defuText.c_str(),
						ImColor(config::esp_distance_color.r, config::esp_distance_color.g, config::esp_distance_color.b),
						12.0f
					);
				}
			}
		}
		// std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

