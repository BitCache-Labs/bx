#include <bx/app.hpp>

int main(int argc, char** args)
{
	bx::app_config_t config{};
	config.width = 800;
	config.height = 600;
	config.title = "bx example basic";

	if (bx::app_init(config) != bx::result_t::OK)
		return EXIT_FAILURE;

	bx::file_add_drive("[local]", BX_EXAMPLES_DIR);

	bx::array<int> indices{ 1, 2, 3, 4, 5 };
	bx::array_fixed<int, 5> indices_f{ 1, 2, 3, 4, 5 };
	bx::array_view<int> indices_v{ indices };

	f32 time = 0;
	u32 frames = 0;
	while (bx::app_begin_frame())
	{
		const f32 dt = (f32)bx::app_frame_time();
		time += dt;
		frames++;

		if (time >= 1.f)
		{
			const f32 fps = frames / time;
			bx_info(bx, "fps: {:.1f}", fps);
			time = fmod(time, 1.f);
			frames = 0;
		}

		bx::app_end_frame(true, false);
	}
	
	bx::app_shutdown();
	return EXIT_SUCCESS;
}