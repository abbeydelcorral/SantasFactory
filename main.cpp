// Santa's Factory - orders, wrapping and automatic color sorting (SFML 3)
// B = belt (left place/rotate, right remove)
// M = wrapper (purple square), S = sorter (cyan diamond)
// Place machines on existing belts. R = restart score/round.
// A wrong-color item arriving at a sorter is recycled automatically.
// Only matching WRAPPED presents count as deliveries.
#include <SFML/Graphics.hpp>
#include <array>
#include <algorithm>
#include <string>
#include <SFML/Audio.hpp>
#include <cmath>
#include <random>
#include <vector>
#include <queue>
#include <cstdint>



int main() {
    constexpr int T = 40, COLS = 20, ROWS = 15;
    constexpr int SX = 2, SY = 7, GX = 17, GY = 7;
    
    // Level progression
    int level = 1;
    int levelGoal = 3;

    constexpr int MAX_LEVEL = 5;



    // Santa's Rush Mode
    constexpr float LEVEL_TIME = 90.f;
    float timeLeft = LEVEL_TIME;

    float levelDuration = LEVEL_TIME;

    int partsLeft = 0;

    // Track belts that the player personally placed.
    std::array<std::array<bool, COLS>, ROWS> playerPlaced{};


    
    constexpr float SPEED = 170.f;
    constexpr float WRAP_SECONDS = 0.45f;

    const int dx[] = {1, 0, -1, 0};
    const int dy[] = {0, 1, 0, -1};

    
sf::RenderWindow window(
    sf::VideoMode({1000, 700}),
    "Santa's Factory"
);

    window.setFramerateLimit(60);

    // =====================================
    // FULLSCREEN DISPLAY
    // =====================================

    bool isFullscreen = false;

    auto updateGameView = [&]() {
        sf::Vector2u size = window.getSize();

        float screenRatio =
            static_cast<float>(size.x) / size.y;

        float gameRatio = 1000.f / 700.f;

        sf::View gameView(
            sf::FloatRect({0.f, 0.f}, {1000.f, 700.f})
        );

        if (screenRatio > gameRatio) {
            float width = gameRatio / screenRatio;
            gameView.setViewport(
                sf::FloatRect(
                    {(1.f - width) / 2.f, 0.f},
                    {width, 1.f}
                )
            );
        }
        else {
            float height = screenRatio / gameRatio;
            gameView.setViewport(
                sf::FloatRect(
                    {0.f, (1.f - height) / 2.f},
                    {1.f, height}
                )
            );
        }

        window.setView(gameView);
    };

    auto toggleFullscreen = [&]() {
        isFullscreen = !isFullscreen;

        if (isFullscreen) {
            window.create(
                sf::VideoMode::getDesktopMode(),
                "Santa's Factory",
                sf::Style::None
            );
            window.setPosition({0, 0});
        }
        else {
            window.create(
                sf::VideoMode({1000, 700}),
                "Santa's Factory"
            );
        }

        window.setFramerateLimit(60);
        updateGameView();
    };

    updateGameView();


    // Christmas factory sound effects
    sf::SoundBuffer deliveryBuffer;
    sf::SoundBuffer rejectBuffer;

    bool deliveryLoaded =
        deliveryBuffer.loadFromFile("assets/delivery.wav");

    bool rejectLoaded =
        rejectBuffer.loadFromFile("assets/reject.wav");

    sf::Sound deliverySound(deliveryBuffer);
    sf::Sound rejectSound(rejectBuffer);

    // Gremlin sabotage sound effect
    sf::SoundBuffer sabotageBuffer;
    bool sabotageLoaded =
        sabotageBuffer.loadFromFile(
            "assets/gremlin_sabotage.wav"
        );

    sf::Sound sabotageSound(sabotageBuffer);
    sabotageSound.setVolume(20.f);


    deliverySound.setVolume(45.f);
    rejectSound.setVolume(35.f);

    // =====================================
    // CHRISTMAS JAZZ BACKGROUND MUSIC
    // =====================================

    sf::Music backgroundMusic;

    bool musicLoaded = backgroundMusic.openFromFile(
        "assets/santas_factory_jazz_tight_groove.wav"
    );

    bool musicMuted = false;
    constexpr float MUSIC_VOLUME = 18.f;

    if (musicLoaded) {
        backgroundMusic.setLooping(true);
        backgroundMusic.setVolume(MUSIC_VOLUME);
    }

    // Shared mute control
    auto toggleMusic = [&]() {
        musicMuted = !musicMuted;

        if (musicLoaded) {
            backgroundMusic.setVolume(
                musicMuted ? 0.f : MUSIC_VOLUME
            );
        }
    };




    // Font for the in-game dashboard
    sf::Font hudFont;
    bool fontLoaded =
        hudFont.openFromFile("C:/Windows/Fonts/arial.ttf");


    std::array<std::array<int, COLS>, ROWS> belts{};
    std::array<std::array<bool, COLS>, ROWS> wrappers{};

    // Boxing stations put raw toys inside gift boxes.
    std::array<std::array<bool, COLS>, ROWS> boxers{};

    std::array<std::array<bool, COLS>, ROWS> sorters{};

    // Factory obstacles that cannot be built over
    std::array<std::array<bool, COLS>, ROWS> blocked{};

    for (auto& row : belts) row.fill(-1);


    int col = SX, row = SY, nextCol = SX, nextRow = SY;
    int presentType = 0;   // 0 = red, 1 = blue
    // Mixed incoming shipments deliberately include colors out of order.
    const int incomingColors[] = {0, 1, 1, 0, 0, 1, 0, 1};
    int spawnIndex = 0;
    int orderType = 0;     // The next color Santa needs
    int delivered = 0, rejected = 0, score = 0, recycled = 0;
    int redStored = 0, blueStored = 0;
    int releaseChoice = -1; // -1 none, 0 red, 1 blue
    float progress = 0.f, wrappingTime = 0.f;
    bool traveling = false, wrapped = false;
    // 0 = belts, 1 = wrapping, 2 = sorting
    int toolMode = 0;
    float sortingPause = 0.f;
    float stuckTime = 0.f;

    sf::Clock clock;

    // Santa character animation
    float santaAnimationTime = 0.f;
    float santaCheerTime = 0.f;


    // Factory economy and employee system
    int money = 0;

    // Santa's special rush orders
    bool rushActive = false;
    
    int rushCount = 0;
    int nextRushAt = 1;

    // A different sequence of surprise rushes each playthrough.
    std::mt19937 rushRandom(std::random_device{}());
    std::uniform_int_distribution<int> rushGap(2, 3);

    float rushTimeLeft = 0.f;
    int rushOrderType = 0;

    // Brief announcement when Santa starts a rush order.
    float rushAnnouncementTime = 0.f;


    constexpr float RUSH_ORDER_TIME = 18.f;
    constexpr int RUSH_BONUS_MONEY = 60;

    constexpr int ELF_COST = 100;

    // Factory supply shop
    constexpr int BELT_PACK_COST = 60;
    constexpr int BELTS_PER_PACK = 3;

    constexpr int CRATE_CLEAR_COST = 80;


    // Machine shop prices
    constexpr int WRAPPER_COST = 80;
    constexpr int SORTER_COST = 100;

    // Free starter equipment for certain levels
    int freeWrappers = 0;


    // Factory upgrades
    int conveyorUpgrade = 0;
    constexpr int MAX_CONVEYOR_UPGRADE = 2;

    // Wrapping and sorting upgrades
   
    // The wrapper now runs at its normal speed.
    // Repair Elf movement can be upgraded instead.
    int wrapperUpgrade = 0;
    int repairUpgrade = 0;
    int sorterUpgrade = 0;
    constexpr int MAX_MACHINE_UPGRADE = 2;



    // =====================================
    // TOY LAUNCHER AND BOXING SYSTEM
    // =====================================

    constexpr float BOX_SECONDS = 0.55f;
    constexpr int LAUNCHER_ELF_COST = 100;
    constexpr int BOXING_ELF_COST = 100;

    bool launcherElfHired = false;
    bool boxingElfHired = false;

    // A new toy waits at the start until launched.
    bool waitingForLaunch = false;
    float launcherDelay = 0.f;

    // Two stages: boxed first, then ribbon-wrapped.
    bool boxed = false;
    bool waitingForBox = false;
    float boxingTime = 0.f;


    bool wrappingElfHired = false;
    bool waitingForWrap = false;

    // Sorting employee
    constexpr int SORTING_ELF_COST = 120;
    bool sortingElfHired = false;
    bool waitingForSort = false;

    // The elf starts near the storage area
    float sortingElfX = 620.f;
    float sortingElfY = 160.f;
    float sortingElfWalkPhase = 0.f;

    // Gremlin sabotage system
    constexpr float GREMLIN_INTERVAL = 24.f;
    float gremlinTimer = 0.f;

    // Only one machine can be broken at a time
    int brokenC = -1;
    int brokenR = -1;

    // Manual repairs take four seconds
    float manualRepairTime = 0.f;

    // Repair elf employee
    constexpr int REPAIR_ELF_COST = 160;
    bool repairElfHired = false;

    float repairElfX = 110.f;
    float repairElfY = 190.f;
    float repairElfWalkPhase = 0.f;

    // Next floor tile each elf is walking toward.
    int repairWaypointC = -1, repairWaypointR = -1;
    int sorterWaypointC = -1, sorterWaypointR = -1;

    // Remember the destination to handle changed orders.
    int repairLastGoalC = -1, repairLastGoalR = -1;
    int sorterLastGoalC = -1, sorterLastGoalR = -1;





    // Start menu and pause state
    bool gameStarted = false;
    bool gamePaused = false;

    // Factory planning and production phases
    bool productionRunning = false;
    
    // One hiring shortcut controls all three elf jobs.
    bool hiringMenuOpen = false;



    auto inside = [](int c, int r) {
        return c >= 0 && c < COLS && r >= 0 && r < ROWS;
    };
    auto resetPresent = [&]() {
        col = SX; row = SY; nextCol = SX; nextRow = SY;
        
        
        progress = 0.f;
        wrappingTime = 0.f;
        boxingTime = 0.f;
        stuckTime = 0.f;

        waitingForWrap = false;
        waitingForBox = false;
        waitingForSort = false;

        // New toys arrive at the loading dock and
        // wait for the player or launcher elf.
        waitingForLaunch = true;
        launcherDelay = 0.75f;

        boxed = false;



        traveling = false; wrapped = false; sortingPause = 0.f;
       
        // Automatically use a stored toy matching
        // Santa's current order before new shipments.
        if (orderType == 0 && redStored > 0) {
            presentType = 0;
            --redStored;
        }
        else if (orderType == 1 && blueStored > 0) {
            presentType = 1;
            --blueStored;
        }
        else {
            // No matching stored toy: use a new shipment.
            ++spawnIndex;
            presentType = incomingColors[spawnIndex % 8];
        }


        releaseChoice = -1;
    };
    

    // Load a factory level
    auto loadLevel = [&](int newLevel) {
        level = newLevel;

        // Reset special orders for the new level
        rushActive = false;
        
        rushCount = 0;
        nextRushAt = 1;

        rushTimeLeft = 0.f;
        rushOrderType = 0;

        rushAnnouncementTime = 0.f;



        // Clear machine damage for the new workshop
        gremlinTimer = 0.f;
        brokenC = -1;
        brokenR = -1;
        manualRepairTime = 0.f;

        repairElfX = 110.f;
        repairElfY = 190.f;
        repairElfWalkPhase = 0.f;

        repairWaypointC = repairWaypointR = -1;
        sorterWaypointC = sorterWaypointR = -1;

        repairLastGoalC = repairLastGoalR = -1;
        sorterLastGoalC = sorterLastGoalR = -1;



        // Each new level begins in Build Mode
        productionRunning = false;

        hiringMenuOpen = false;




        // Reset Rush Mode
        
        // Harder workshops have longer production shifts.
        levelDuration =
            level == 5 ? 140.f :
            level == 4 ? 120.f :
                         LEVEL_TIME;

        timeLeft = levelDuration;


        for (auto& r : playerPlaced) {
            r.fill(false);
        }

        // Limited conveyor pieces for each level
        if (level == 1) {
            partsLeft = 2;
        }
        else if (level == 2) {
            partsLeft = 3;
        }
        else {
            partsLeft = 3;
        }


        // Clear the previous factory
        for (auto& r : belts) r.fill(-1);
        for (auto& r : wrappers) r.fill(false);
        for (auto& r : boxers) r.fill(false);
        for (auto& r : sorters) r.fill(false);

        for (auto& r : blocked) r.fill(false);

        // =====================================
        // LEVEL DESIGN: CONNECTED BELT SEGMENTS
        // =====================================
        // Builds a straight segment from the starting
        // tile up to, but not including, the endpoint.
        // The next segment supplies the turning tile.

        auto beltSegment = [&](int x1, int y1,
                               int x2, int y2) {

            int stepX = (x2 > x1) - (x2 < x1);
            int stepY = (y2 > y1) - (y2 < y1);

            int direction =
                stepX > 0 ? 0 :
                stepY > 0 ? 1 :
                stepX < 0 ? 2 : 3;

            for (int x = x1, y = y1;
                 x != x2 || y != y2;
                 x += stepX, y += stepY) {

                belts[y][x] = direction;
            }
        };




        // Reset level progress
        delivered = 0;
        rejected = 0;
        score = 0;
        recycled = 0;
        redStored = 0;
        blueStored = 0;
        releaseChoice = -1;
        orderType = 0;
        spawnIndex = -1;
        toolMode = 0;

        // Level 2 includes one free wrapping station.
        // All other additional machines must be purchased.
        freeWrappers = 0;


       
        if (level == 1) {
            // LEVEL 1: Route around the storage crates
            levelGoal = 3;
            partsLeft = 6;

            // First part of the conveyor
            for (int c = SX; c <= 8; ++c) {
                belts[SY][c] = 0;
            }

            // Second part, leading to delivery
            for (int c = 12; c < GX; ++c) {
                belts[SY][c] = 0;
            }

            // Crates block the direct route
            blocked[7][9] = true;
            blocked[7][10] = true;
            blocked[7][11] = true;

            // Existing factory machinery
            boxers[7][5] = true;
            wrappers[SY][7] = true;
            sorters[SY][14] = true;
        }

        
        else if (level == 2) {
            // LEVEL 2: The Upper Workshop
            // Route around the storage area and install a wrapper.
            
            levelGoal = 5;
            partsLeft = 6;


            // Starting conveyor
            for (int c = SX; c <= 5; ++c) {
                belts[7][c] = 0;
            }

            // Turn upward
            belts[7][6] = 3;
            belts[6][6] = 3;
            belts[5][6] = 0;

            // Upper conveyor
            for (int c = 7; c <= 13; ++c) {
                belts[5][c] = 0;
            }

            // Turn downward
            belts[5][14] = 1;
            belts[6][14] = 1;
            belts[7][14] = 0;

            // Conveyor toward delivery
            belts[7][15] = 0;
            belts[7][16] = 0;

            
            // Five missing conveyor pieces
            // Players must rebuild both horizontal
            // and vertical sections correctly.
            belts[5][8] = -1;
            belts[5][11] = -1;
            belts[7][15] = -1;
            belts[7][6] = -1;
            belts[6][14] = -1;


            // Storage crates obstruct the direct route
            for (int c = 8; c <= 12; ++c) {
                blocked[7][c] = true;
                blocked[8][c] = true;
            }

            for (int c = 9; c <= 11; ++c) {
                blocked[6][c] = true;
            }
            boxers[7][4] = true;

           
            // Both machines are ready before production.
            // No machine purchase required to solve Level 2.
            wrappers[5][9] = true;
            sorters[5][13] = true;

        }
        
        // =====================================
        // LEVEL 3: THE SPIRAL WORKSHOP
        // =====================================
        else if (level == 3) {
            levelGoal = 7;
            partsLeft = 7;

            // Route wraps around central storage:
            // right, up, across, down, across, up.
            beltSegment(2, 7, 5, 7);
            beltSegment(5, 7, 5, 3);
            beltSegment(5, 3, 10, 3);
            beltSegment(10, 3, 10, 10);
            beltSegment(10, 10, 15, 10);
            beltSegment(15, 10, 15, 7);
            beltSegment(15, 7, 17, 7);

            // Six missing pieces.
            belts[7][3] = -1;
            belts[5][5] = -1;
            belts[3][7] = -1;
            belts[6][10] = -1;
            belts[10][12] = -1;
            belts[8][15] = -1;

           
            // Warehouse shelving forms an L-shaped
            // inner wall beside the winding conveyor.
            for (int r = 6; r <= 9; ++r) {
                for (int c = 6; c <= 8; ++c) {
                    blocked[r][c] = true;
                }
            }
            for (int c = 6; c <= 9; ++c) {
                blocked[9][c] = true;
            }

            // Storage partitions beside the lower route.
            for (int r = 7; r <= 9; ++r) {
                blocked[r][12] = true;
            }
            for (int c = 11; c <= 14; ++c) {
                blocked[9][c] = true;
            }

            // Leave a walkway above the sorting station
            // so the Sorting Elf can reach it faster.
            blocked[9][13] = false;


            // Constrain the upper conveyor gap.
            blocked[4][7] = true;
            blocked[4][8] = true;




            // Three stages of production.
            boxers[7][4] = true;
            wrappers[3][8] = true;
            sorters[10][13] = true;
        }

        // =====================================
        // LEVEL 4: THE WAREHOUSE CHALLENGE
        // =====================================
        else if (level == 4) {
            levelGoal = 6;
            partsLeft = 7;

            // A different layout: go down into the
            // warehouse, cross a lower corridor,
            // then climb back toward delivery.
            beltSegment(2, 7, 5, 7);
            beltSegment(5, 7, 5, 11);
            beltSegment(5, 11, 12, 11);
            beltSegment(12, 11, 12, 6);
            beltSegment(12, 6, 16, 6);
            beltSegment(16, 6, 16, 7);
            beltSegment(16, 7, 17, 7);

            // Six missing pieces; one spare.
            belts[7][4] = -1;
            belts[9][5] = -1;
            belts[11][7] = -1;
            belts[9][12] = -1;
            belts[6][14] = -1;
            belts[7][16] = -1;

           
            // U-shaped warehouse storage aisle.
            for (int r = 7; r <= 10; ++r) {
                blocked[r][7] = true;
            }

            for (int c = 7; c <= 10; ++c) {
                blocked[10][c] = true;
            }

            for (int r = 7; r <= 9; ++r) {
                blocked[r][10] = true;
            }

            // Smaller storage wall near the return path.
            for (int r = 8; r <= 10; ++r) {
                blocked[r][14] = true;
            }

            for (int c = 13; c <= 15; ++c) {
                blocked[10][c] = true;
            }

            // Obstacle beside a vertical conveyor gap.
            blocked[9][11] = true;



            // Equipment spread across the route.
            boxers[7][3] = true;
            wrappers[11][9] = true;
            sorters[6][15] = true;
        }

        // =====================================
        // LEVEL 5: CHRISTMAS EVE FINALE
        // =====================================
        else if (level == 5) {
            levelGoal = 8;
            partsLeft = 10;

            // The grand winding production line:
            // climbs to the upper floor, descends
            // through the workshop, crosses the
            // lower floor, then returns to Santa.
            beltSegment(2, 7, 4, 7);
            beltSegment(4, 7, 4, 3);
            beltSegment(4, 3, 7, 3);
            beltSegment(7, 3, 7, 11);
            beltSegment(7, 11, 12, 11);
            beltSegment(12, 11, 12, 5);
            beltSegment(12, 5, 16, 5);
            beltSegment(16, 5, 16, 7);
            beltSegment(16, 7, 17, 7);

            // Nine gaps; one spare conveyor.
            belts[5][4] = -1;
            belts[3][5] = -1;
            belts[6][7] = -1;
            belts[9][7] = -1;
            belts[11][9] = -1;
            belts[9][12] = -1;
            belts[6][12] = -1;
            belts[5][13] = -1;
            belts[6][16] = -1;

           
            // Shelving maze through the central workshop.
            for (int r = 7; r <= 9; ++r) {
                blocked[r][9] = true;
            }

            for (int c = 8; c <= 11; ++c) {
                blocked[9][c] = true;
            }

            for (int r = 7; r <= 8; ++r) {
                blocked[r][11] = true;
            }

            // Right-side warehouse aisle.
            for (int r = 8; r <= 10; ++r) {
                blocked[r][14] = true;
            }

            for (int c = 13; c <= 15; ++c) {
                blocked[10][c] = true;
            }

            // Crates near the upper and return gaps
            // create narrower construction corridors.
            blocked[4][5] = true;
            blocked[4][6] = true;
            blocked[6][11] = true;



            // Machine positions are spread out
            // across the entire finale route.
            boxers[7][3] = true;
            wrappers[11][10] = true;
            sorters[5][14] = true;
        }




        
        // First toy is ready automatically.
        // Later toys must be launched by the player
        // or an automated launcher elf.
        resetPresent();
        waitingForLaunch = false;

    };

    // Start Level 1
    loadLevel(1);

    // Finish repairs and resume production
    auto finishRepair = [&]() {
        if (brokenC < 0) return;

        bool presentAtMachine =
            col == brokenC && row == brokenR;

        brokenC = -1;
        brokenR = -1;
        manualRepairTime = 0.f;

        // A hired wrapping elf resumes work automatically
        if (presentAtMachine &&
            waitingForWrap &&
            wrappingElfHired) {

            waitingForWrap = false;
            
wrappingTime = WRAP_SECONDS /
               (1.f + 0.5f * wrapperUpgrade);

        }
    };

    // =====================================
    // FIVE-WORKER HIRING SYSTEM
    // =====================================

    auto hireEmployee = [&](int employee) -> bool {
        if (!gameStarted || gamePaused ||
            timeLeft <= 0.f || delivered >= levelGoal) {
            return false;
        }

        switch (employee) {
            case 1:  // Launcher Elf
                if (launcherElfHired ||
                    money < LAUNCHER_ELF_COST) return false;

                money -= LAUNCHER_ELF_COST;
                launcherElfHired = true;
                break;

            case 2:  // Boxing Elf
                if (boxingElfHired ||
                    money < BOXING_ELF_COST) return false;

                money -= BOXING_ELF_COST;
                boxingElfHired = true;

                
                if (waitingForBox &&
                    !(col == brokenC && row == brokenR)) {

                    waitingForBox = false;
                    boxingTime = BOX_SECONDS;
                }

                break;

            case 3:  // Wrapping Elf
                if (wrappingElfHired ||
                    money < ELF_COST) return false;

                money -= ELF_COST;
                wrappingElfHired = true;

                if (waitingForWrap &&
                    !(col == brokenC && row == brokenR)) {
                    waitingForWrap = false;
                    wrappingTime = WRAP_SECONDS /
                        (1.f + 0.5f * wrapperUpgrade);
                }
                break;

            case 4:  // Sorting Elf
                if (sortingElfHired ||
                    money < SORTING_ELF_COST) return false;

                money -= SORTING_ELF_COST;
                sortingElfHired = true;
                break;

            case 5:  // Repair Elf
                if (repairElfHired ||
                    money < REPAIR_ELF_COST) return false;

                money -= REPAIR_ELF_COST;
                repairElfHired = true;
                break;

            default:
                return false;
        }

        return true;
    };


   
    // =====================================
    // DIRECT FACTORY UPGRADES
    // =====================================
    // 0 = conveyor, 1 = repair elf, 2 = sorting elf

    auto purchaseUpgrade = [&](int upgradeType) -> bool {
        if (!gameStarted || gamePaused ||
            hiringMenuOpen ||
            timeLeft <= 0.f ||
            delivered >= levelGoal) {
            return false;
        }

        int* currentLevel = nullptr;
        int cost = 0;
        int maxLevel = 2;

        if (upgradeType == 0) {
            currentLevel = &conveyorUpgrade;
            cost = 80 * (conveyorUpgrade + 1);
        }
        else if (upgradeType == 1) {
            if (!repairElfHired) return false;

            currentLevel = &repairUpgrade;
            cost = 70 * (repairUpgrade + 1);
        }
        else if (upgradeType == 2) {
            if (!sortingElfHired) return false;

            currentLevel = &sorterUpgrade;
            cost = 70 * (sorterUpgrade + 1);
        }
        else {
            return false;
        }

        if (*currentLevel >= maxLevel || money < cost) {
            return false;
        }

        money -= cost;
        ++(*currentLevel);
        return true;
    };




    // ============================================
    // WALKABLE FLOOR PATHFINDING FOR FACTORY ELVES
    // ============================================

    auto moveElfOnFloor = [&](
        float& ex, float& ey,
        int& waypointC, int& waypointR,
        int& lastGoalC, int& lastGoalR,
        int goalC, int goalR,
        bool stopBesideMachine,
        float speed, float elapsed,
        float& walkPhase
    ) -> bool {

        // Recalculate when the elf gets a new job.
        if (goalC != lastGoalC || goalR != lastGoalR) {
            waypointC = waypointR = -1;
            lastGoalC = goalC;
            lastGoalR = goalR;
        }

       
        auto walkable = [&](int c, int r) -> bool {
            if (!inside(c, r)) return false;

            // Christmas tree areas are off-limits.
            bool leftTree =
                c <= 2 && r >= 9 && r <= 12;

            bool rightTree =
                c >= 17 && r >= 9 && r <= 12;

            if (leftTree || rightTree) {
                return false;
            }

            // Avoid warehouse storage and machinery.
            return !blocked[r][c] &&
                   belts[r][c] < 0 &&
                   !boxers[r][c] &&
                   !wrappers[r][c] &&
                   !sorters[r][c];
        };


        // Calculate a new waypoint only when needed.
        if (waypointC < 0) {

            int startC = std::clamp(
                static_cast<int>(ex / T), 0, COLS - 1
            );
            int startR = std::clamp(
                static_cast<int>(ey / T), 0, ROWS - 1
            );

            std::array<std::array<int, COLS>, ROWS> distance;
            std::array<
                std::array<sf::Vector2i, COLS>, ROWS
            > previous;

            for (auto& r : distance) r.fill(-1);
            for (auto& r : previous) {
                r.fill(sf::Vector2i(-1, -1));
            }

            std::queue<sf::Vector2i> search;
            search.push({startC, startR});
            distance[startR][startC] = 0;

            // Breadth-first search through empty floor.
            while (!search.empty()) {
                sf::Vector2i current = search.front();
                search.pop();

                for (int d = 0; d < 4; ++d) {
                    int nc = current.x + dx[d];
                    int nr = current.y + dy[d];

                    if (!walkable(nc, nr)) continue;
                    if (distance[nr][nc] >= 0) continue;

                    distance[nr][nc] =
                        distance[current.y][current.x] + 1;

                    previous[nr][nc] = current;
                    search.push({nc, nr});
                }
            }

            int destinationC = -1;
            int destinationR = -1;
            int shortest = ROWS * COLS + 1;

            auto consider = [&](int c, int r) {
                if (!inside(c, r)) return;
                if (distance[r][c] < 0) return;

                if (distance[r][c] < shortest) {
                    shortest = distance[r][c];
                    destinationC = c;
                    destinationR = r;
                }
            };

            if (stopBesideMachine) {
                // Work from the nearest accessible
                // tile beside the machine.
                for (int d = 0; d < 4; ++d) {
                    consider(
                        goalC + dx[d],
                        goalR + dy[d]
                    );
                }
            }
            else {
                // Returning home.
                consider(goalC, goalR);
            }

            // No safe floor route available.
            if (destinationC < 0) return false;

            sf::Vector2i next(
                destinationC, destinationR
            );

            // Trace backward to find the first step.
            while (!(next.x == startC &&
                     next.y == startR)) {

                sf::Vector2i parent =
                    previous[next.y][next.x];

                if (parent.x == startC &&
                    parent.y == startR) {
                    break;
                }

                if (parent.x < 0) return false;

                next = parent;
            }

            waypointC = next.x;
            waypointR = next.y;
        }

        float targetX = waypointC * T + T / 2.f;
        float targetY = waypointR * T + T / 2.f;

        float diffX = targetX - ex;
        float diffY = targetY - ey;
        float distanceToWaypoint = std::sqrt(
            diffX * diffX + diffY * diffY
        );

        float movement = speed * elapsed;

        if (distanceToWaypoint > movement &&
            distanceToWaypoint > 0.02f) {

            ex += diffX / distanceToWaypoint * movement;
            ey += diffY / distanceToWaypoint * movement;

            if (movement > 0.f) {
                walkPhase += elapsed * 12.f;
            }

            return false;
        }

        // Finish this tile before choosing another one.
        ex = targetX;
        ey = targetY;
        waypointC = waypointR = -1;

        int currentC = static_cast<int>(ex / T);
        int currentR = static_cast<int>(ey / T);

        if (stopBesideMachine) {
            return std::abs(currentC - goalC) +
                   std::abs(currentR - goalR) == 1;
        }

        return currentC == goalC &&
               currentR == goalR;
    };



    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
             
                // F11 toggles fullscreen mode.
                if (key->code == sf::Keyboard::Key::F11) {
                    toggleFullscreen();
                    continue;
                }
  
    // ENTER starts the game
    if (key->code == sf::Keyboard::Key::Enter &&
        !gameStarted) {
       
        gameStarted = true;
        gamePaused = false;

        if (musicLoaded) {
            backgroundMusic.play();
        }

        clock.restart();

    }

    // ESC pauses and resumes the game
    if (key->code == sf::Keyboard::Key::Escape &&
        gameStarted &&
        delivered < levelGoal &&
        timeLeft > 0.f) {
        
        gamePaused = !gamePaused;

        if (musicLoaded) {
            if (gamePaused) {
                backgroundMusic.pause();
            }
            else {
                backgroundMusic.play();
            }
        }

        clock.restart();

    }

    // Ignore gameplay keys on menu or while paused
    if (!gameStarted || gamePaused) {
        continue;
    }


                // =====================================
                // SPACE: MANUAL PRODUCTION
                // =====================================

                if (key->code == sf::Keyboard::Key::Space &&
                    timeLeft > 0.f &&
                    productionRunning) {

                    // First stage: place the toy into a box.
                   
                    if (waitingForBox &&
                        !(col == brokenC && row == brokenR)) {

                        waitingForBox = false;
                        boxingTime = BOX_SECONDS;
                    }


                    // Second stage: add ribbon and bow.
                    else if (waitingForWrap &&
                             boxed &&
                             !(col == brokenC &&
                               row == brokenR)) {

                        waitingForWrap = false;

                        wrappingTime = WRAP_SECONDS /
                            (1.f + 0.5f * wrapperUpgrade);
                    }
                }




               
                // =====================================
                // EMPLOYEE HIRING SHORTCUTS
                // =====================================

                // H opens or closes the employee shop.
                if (key->code == sf::Keyboard::Key::H) {
                    hiringMenuOpen = !hiringMenuOpen;
                    continue;
                }

                // When the shop is open, 1-5 select employees.
                if (hiringMenuOpen) {
                    int employee = 0;

                    if (key->code == sf::Keyboard::Key::Num1)
                        employee = 1;
                    else if (key->code == sf::Keyboard::Key::Num2)
                        employee = 2;
                    else if (key->code == sf::Keyboard::Key::Num3)
                        employee = 3;
                    else if (key->code == sf::Keyboard::Key::Num4)
                        employee = 4;
                    else if (key->code == sf::Keyboard::Key::Num5)
                        employee = 5;

                    if (employee > 0 && hireEmployee(employee)) {
                        hiringMenuOpen = false;
                    }

                    // Prevent shop keys from affecting gameplay.
                    continue;
                }


                // E: Send the next toy into production
                if (key->code == sf::Keyboard::Key::E &&
                    waitingForLaunch &&
                    productionRunning &&
                    delivered < levelGoal &&
                    timeLeft > 0.f) {

                    waitingForLaunch = false;
                }
                

                // Direct upgrades: no tool selection required.
                if (key->code == sf::Keyboard::Key::B) {
                    purchaseUpgrade(0);
                }

                if (key->code == sf::Keyboard::Key::Z) {
                    purchaseUpgrade(1);
                }

                if (key->code == sf::Keyboard::Key::T) {
                    purchaseUpgrade(2);
                }


                
                // S: Store the waiting present
               
                if (key->code == sf::Keyboard::Key::S &&
                    waitingForSort &&
                    !sortingElfHired &&
                    !(col == brokenC && row == brokenR) &&
                    timeLeft > 0.f) {


                    waitingForSort = false;
                    
sortingPause = 0.35f /
               (1.f + 0.5f * sorterUpgrade);

                }

                // A: Approve the present and send it onward
                
                if (key->code == sf::Keyboard::Key::A &&
                    waitingForSort &&
                    !sortingElfHired &&
                    !(col == brokenC && row == brokenR) &&
                    timeLeft > 0.f) {


                    waitingForSort = false;
                }
                

                // P: Buy three additional conveyor pieces
                if (key->code == sf::Keyboard::Key::P &&
                    money >= BELT_PACK_COST &&
                    timeLeft > 0.f) {

                    money -= BELT_PACK_COST;
                    partsLeft += BELTS_PER_PACK;
                }

                // G: Start factory production after building
                if (key->code == sf::Keyboard::Key::G &&
                    !productionRunning &&
                    delivered < levelGoal &&
                    timeLeft > 0.f) {

                    productionRunning = true;
                    clock.restart();
                }

                // X: Start manually repairing a broken machine
                if (key->code == sf::Keyboard::Key::X &&
                    brokenC >= 0 &&
                    manualRepairTime <= 0.f &&
                    timeLeft > 0.f) {

                    manualRepairTime = 4.f;
                }

                
                // Intuitive factory shortcuts
                if (key->code == sf::Keyboard::Key::M) {
                    toggleMusic();
                }

                // Restart current level
                if (key->code == sf::Keyboard::Key::R) {
                    loadLevel(level);
                }

                // Advance to next level
                if (key->code == sf::Keyboard::Key::N &&
                    delivered >= levelGoal &&
                    
level < MAX_LEVEL) {
    loadLevel(level + 1);

                }

            }
            if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {

    // Prevent building while on menu or paused
    if (!gameStarted || gamePaused) {
        continue;
    }
               

                // =====================================
                // SIDEBAR MOUSE CONTROLS
                // =====================================
                // The factory occupies x = 0 to 799.
                // The new control panel starts at x = 800.

                
int mx = static_cast<int>(
    window.mapPixelToCoords(mouse->position).x
);

int my = static_cast<int>(
    window.mapPixelToCoords(mouse->position).y
);


                // =====================================
                // EMPLOYEE SHOP MOUSE CONTROLS
                // =====================================

                if (hiringMenuOpen) {
                    if (mouse->button ==
                        sf::Mouse::Button::Left) {

                        // Five employee choices, each 52px apart.
                        for (int i = 0; i < 5; ++i) {
                            float top = 210.f + i * 52.f;

                            if (mx >= 190 && mx < 610 &&
                                my >= top && my < top + 40.f) {

                                if (hireEmployee(i + 1)) {
                                    hiringMenuOpen = false;
                                }
                                break;
                            }
                        }

                        // Click outside the shop to close it.
                        if (mx < 160 || mx > 640 ||
                            my < 150 || my > 500) {
                            hiringMenuOpen = false;
                        }
                    }

                    continue;
                }
                // Clickable music control
                if (mouse->button ==
                        sf::Mouse::Button::Left &&
                    mx >= 570 && mx < 774 &&
                    my >= 657 && my < 693) {

                    toggleMusic();
                    continue;
                }



                if (mx >= 800) {
                    
if (mouse->button == sf::Mouse::Button::Left &&
    delivered < levelGoal && timeLeft > 0.f) {


                       
                        // Three independent upgrade buttons.
                        if (mx >= 812 && mx < 988) {
                            if (my >= 295 && my < 326) {
                                purchaseUpgrade(0); // Conveyor
                            }
                            else if (my >= 332 && my < 363) {
                                purchaseUpgrade(1); // Repair Elf
                            }
                            else if (my >= 369 && my < 400) {
                                purchaseUpgrade(2); // Sorting Elf
                            }

                            // Purchase 3 additional belts
                            else if (my >= 466 && my < 503) {
                                if (money >= BELT_PACK_COST) {
                                    money -= BELT_PACK_COST;
                                    partsLeft += BELTS_PER_PACK;
                                }
                            }

                            // Start production
                            else if (my >= 517 && my < 559) {
                                if (!productionRunning) {
                                    productionRunning = true;
                                    clock.restart();
                                }
                            }

                           
                            // Open the unified employee shop.
                            else if (my >= 601 && my < 694) {
                                hiringMenuOpen = true;
                            }

                        }
                    }

                    // Do not treat sidebar clicks as grid clicks.
                    continue;
                }

                // Bottom control strip is not buildable.
                if (my >= 600) {
                    continue;
                }


                // Freeze building after winning or losing.
                if (delivered >= levelGoal ||
                    timeLeft <= 0.f) {
                    continue;
                }

               
                
sf::Vector2f gameMouse =
    window.mapPixelToCoords(mouse->position);

int c = static_cast<int>(gameMouse.x / T);
int r = static_cast<int>(gameMouse.y / T);

                if (!inside(c, r) || (c == SX && r == SY) ||
                    (c == GX && r == GY)) continue;

                
                // Clear warehouse crates to expand the factory.
                // Clearing is only available during Build Mode.
                if (blocked[r][c]) {

                    if (!productionRunning &&
                        mouse->button == sf::Mouse::Button::Left &&
                        money >= CRATE_CLEAR_COST) {

                        money -= CRATE_CLEAR_COST;
                        blocked[r][c] = false;
                    }

                    continue;
                }


                bool left = mouse->button == sf::Mouse::Button::Left;
                bool right = mouse->button == sf::Mouse::Button::Right;
                
                
                // Conveyor placement is always available.
                // Wrapping and sorting machines are preinstalled.

                if (left) {
                    if (belts[r][c] == -1) {
                        // Place a new conveyor.
                        if (partsLeft > 0) {
                            belts[r][c] = 0;
                            playerPlaced[r][c] = true;
                            --partsLeft;
                        }
                    }
                    else {
                        // Rotate an existing conveyor.
                        belts[r][c] =
                            (belts[r][c] + 1) % 4;
                    }
                }

                if (right && belts[r][c] >= 0) {
                    // Refund only player-placed belts.
                    if (playerPlaced[r][c]) {
                        ++partsLeft;
                    }

                    playerPlaced[r][c] = false;
                    belts[r][c] = -1;
                }


            }
        }

        float dt = std::min(clock.restart().asSeconds(), 0.05f);

    // Stop game time while on menu or paused
   
    if (!gameStarted || gamePaused || hiringMenuOpen) {
        dt = 0.f;
    }




        // Countdown for the brief rush-order announcement.
        if (rushAnnouncementTime > 0.f) {
            rushAnnouncementTime =
                std::max(0.f, rushAnnouncementTime - dt);
        }


        // Santa's idle animation and delivery celebration
        santaAnimationTime += dt;

        if (santaCheerTime > 0.f) {
            santaCheerTime -= dt;

            if (santaCheerTime < 0.f) {
                santaCheerTime = 0.f;
            }
        }


        // Count down while the level is active.
        

if (gameStarted && !gamePaused &&
    productionRunning &&
    delivered < levelGoal && timeLeft > 0.f) {


            timeLeft -= dt;

            if (timeLeft < 0.f) {
                timeLeft = 0.f;
            }
        }

        
        

if (gameStarted && !gamePaused &&
    productionRunning &&
    delivered < levelGoal && timeLeft > 0.f) {

            // Santa's rush order countdown
            if (rushActive) {
                rushTimeLeft -= dt;

                if (rushTimeLeft <= 0.f) {
                    rushTimeLeft = 0.f;
                    rushActive = false;
                }
            }


            // =====================================
            // AUTOMATIC TOY LAUNCHER
            // =====================================
            if (launcherElfHired && waitingForLaunch) {
                launcherDelay -= dt;

                if (launcherDelay <= 0.f) {
                    waitingForLaunch = false;
                    launcherDelay = 0.f;
                }
            }

            // A hired boxing elf can take over immediately
            // if a toy is already waiting at the station.
            if (boxingElfHired && waitingForBox &&
    !(col == brokenC && row == brokenR)) {
                waitingForBox = false;
                boxingTime = BOX_SECONDS;
            }




            // =====================================
            // GREMLINS AND REPAIR ELF AI
            // =====================================

           
            // =====================================
            // RANDOM MULTI-MACHINE SABOTAGE
            // =====================================

            gremlinTimer += dt;

            if (gremlinTimer >= GREMLIN_INTERVAL) {
                gremlinTimer = 0.f;

                // Only one sabotaged machine at a time.
                if (brokenC < 0) {
                    std::vector<sf::Vector2i> targets;

                    for (int r = 0; r < ROWS; ++r) {
                        for (int c = 0; c < COLS; ++c) {
                            if (boxers[r][c] ||
                                wrappers[r][c] ||
                                sorters[r][c]) {

                                targets.push_back({c, r});
                            }
                        }
                    }

                    if (!targets.empty()) {
                        std::uniform_int_distribution<int> pick(
                            0,
                            static_cast<int>(targets.size()) - 1
                        );

                        sf::Vector2i target =
                            targets[pick(rushRandom)];

                        brokenC = target.x;
                        brokenR = target.y;
                        manualRepairTime = 0.f;

                        if (sabotageLoaded) {
                            sabotageSound.play();
                        }

                    }
                }
            }



            // Manual repairs count down after pressing X.
            if (brokenC >= 0 && manualRepairTime > 0.f) {
                manualRepairTime -= dt;

                if (manualRepairTime <= 0.f) {
                    finishRepair();
                }
            }

           
            // =====================================
            // REPAIR ELF: OBSTACLE-AWARE MOVEMENT
            // =====================================

            if (repairElfHired) {

                bool hasRepairJob = brokenC >= 0;

                int goalC = hasRepairJob ? brokenC : 2;
                int goalR = hasRepairJob ? brokenR : 4;

                float repairSpeed =
                    165.f * (1.f + 0.4f * repairUpgrade);

                bool reached = moveElfOnFloor(
                    repairElfX, repairElfY,
                    repairWaypointC, repairWaypointR,
                    repairLastGoalC, repairLastGoalR,
                    goalC, goalR,
                    hasRepairJob,
                    repairSpeed, dt,
                    repairElfWalkPhase
                );

                if (reached && hasRepairJob) {
                    finishRepair();
                }
            }







          
            // =====================================
            // SORTING ELF: OBSTACLE-AWARE MOVEMENT
            // =====================================

            if (sortingElfHired) {

                int goalC = waitingForSort ? col : 15;
                int goalR = waitingForSort ? row : 4;

                float sortSpeed =
                    140.f *
                    (1.f + 0.35f * sorterUpgrade);

                bool reached = moveElfOnFloor(
                    sortingElfX, sortingElfY,
                    sorterWaypointC, sorterWaypointR,
                    sorterLastGoalC, sorterLastGoalR,
                    goalC, goalR,
                    waitingForSort,
                    sortSpeed, dt,
                    sortingElfWalkPhase
                );

                if (reached && waitingForSort &&
                    !(col == brokenC && row == brokenR)) {

                    waitingForSort = false;

                    if (presentType != orderType) {
                        // Incorrect color: put it in storage.
                        sortingPause =
                            0.35f /
                            (1.f + 0.5f * sorterUpgrade);
                    }
                    // Otherwise, the matching gift proceeds.
                }
            }




           
            if (sortingPause > 0.f) {
                if (!(col == brokenC && row == brokenR)) {
                    sortingPause -= dt;
                }

                if (sortingPause <= 0.f) {
                    sortingPause = 0.f;
                    ++recycled;
                    if (presentType == 0) ++redStored;
                    else ++blueStored;
                    resetPresent(); // Wrong color goes into color storage
                }


            
            } else if (boxingTime > 0.f) {
                if (!(col == brokenC && row == brokenR)) {
                    boxingTime -= dt;
                }


                if (boxingTime <= 0.f) {
                    boxingTime = 0.f;
                    boxed = true;
                }

            
            } else if (wrappingTime > 0.f) {
                if (!(col == brokenC && row == brokenR)) {
                    wrappingTime -= dt;
                }

                if (wrappingTime <= 0.f) { wrappingTime = 0.f; wrapped = true; }
            } else {
                


if (!traveling &&
    !waitingForLaunch &&
    !waitingForBox &&
    !waitingForWrap &&
    !waitingForSort) {



                    int direction = belts[row][col];
                    if (direction >= 0) {
                        int c = col + dx[direction], r = row + dy[direction];
                        if (inside(c, r) &&
                            (belts[r][c] >= 0 || (c == GX && r == GY))) {
                            nextCol = c; nextRow = r;
                            traveling = true; progress = 0.f;
                        }
                    }
                }
                if (traveling) {
                    
progress +=
    SPEED * (1.f + 0.25f * conveyorUpgrade) * dt / T;

                    if (progress >= 1.f) {
                        col = nextCol; row = nextRow;
                        progress = 0.f; traveling = false;
                        if (col == GX && row == GY) {
                            
if (boxed && wrapped && presentType == orderType) {


                                ++delivered; score += 100;

santaCheerTime = 1.5f;


money += 40;

                                // Successful rush delivery bonus
                                if (rushActive &&
                                    rushTimeLeft > 0.f) {

                                    money += RUSH_BONUS_MONEY;
                                    score += 100;
                                    rushActive = false;
                                }



if (deliveryLoaded) {
    deliverySound.play();
}

                                orderType = 1 - orderType;

                              
                                // Santa's scheduled surprise rush orders
                                const int maxRushOrders =
                                    level == 1 ? 1 :
                                    level <= 3 ? 2 : 3;

                                if (!rushActive &&
                                    rushCount < maxRushOrders &&
                                    delivered >= nextRushAt &&
                                    delivered < levelGoal) {

                                    rushActive = true;
                                    rushTimeLeft =
                                        level >= 4 ? 24.f :
                                                     RUSH_ORDER_TIME;

                                    rushOrderType = orderType;
                                    ++rushCount;

                                    rushAnnouncementTime = 1.6f;


                                    santaCheerTime = 1.2f;


                                    // Schedule another opportunity
                                    // two or three deliveries later.
                                    nextRushAt =
                                        delivered + rushGap(rushRandom);
                                }


                            } else {
                                ++rejected; score = std::max(0, score - 20);

if (rejectLoaded) {
    rejectSound.play();
}

                            }
                            if (delivered < levelGoal) resetPresent();
                        


} else if (sorters[row][col]) {
    // Stop for inspection, even if the scanner is damaged.
    // Neither manual sorting nor the elf can operate
    // until this machine is repaired.
    waitingForSort = true;



                        

} else if (boxers[row][col] && !boxed) {

    bool brokenHere =
        col == brokenC && row == brokenR;

    // Never operate a damaged boxing station.
    if (brokenHere) {
        waitingForBox = true;
    }
    else if (boxingElfHired) {
        boxingTime = BOX_SECONDS;
    }
    else {
        waitingForBox = true;
    }




} else if (wrappers[row][col] && !wrapped) {

    bool brokenHere =
        col == brokenC && row == brokenR;

    if (brokenHere) {
        // Broken machine: wait for a repair
        waitingForWrap = true;
    }
    else if (wrappingElfHired) {
        
wrappingTime = WRAP_SECONDS /
               (1.f + 0.5f * wrapperUpgrade);

    }
    else {
        waitingForWrap = true;
    }
}


                    }
                }
            }
        }

        // Detect presents stuck at the end of a conveyor.
        

if (gameStarted && !gamePaused && productionRunning &&
    delivered < levelGoal &&
    timeLeft > 0.f &&
    !traveling &&
    
    !waitingForLaunch &&
    !waitingForBox &&
    !waitingForWrap &&
    !waitingForSort &&
    boxingTime <= 0.f &&
    wrappingTime <= 0.f &&
    sortingPause <= 0.f) {


            int direction = belts[row][col];
            bool canMove = false;

            if (direction >= 0) {
                int c = col + dx[direction];
                int r = row + dy[direction];

                canMove = inside(c, r) &&
                    (belts[r][c] >= 0 ||
                     (c == GX && r == GY));
            }

            if (!canMove) {
                stuckTime += dt;

                // After 1.5 seconds, reject and respawn.
                if (stuckTime >= 1.5f) {
                    ++rejected;
                    score = std::max(0, score - 20);
                    resetPresent();
                }
            } else {
                stuckTime = 0.f;
            }
        } else {
            stuckTime = 0.f;
        }


        
        
        // Santa's Rush Mode HUD
        std::string colorName =
            orderType == 0 ? "RED" : "BLUE";

        int secondsLeft =
            static_cast<int>(timeLeft + 0.999f);

        std::string title =
            "Santa's Factory | Level " +
            std::to_string(level) +
            "/5 | TIME: " +
            std::to_string(secondsLeft) + "s" +
            " | Parts: " +
            std::to_string(partsLeft) +
            " | Order: " + colorName +
            " | Delivered: " +
            std::to_string(delivered) + "/" +
            std::to_string(levelGoal) +
            " | Score: " +
            std::to_string(score);

        if (delivered >= levelGoal) {
            if (level < MAX_LEVEL) {
                title =
                    "LEVEL " + std::to_string(level) +
                    " COMPLETE! Time left: " +
                    std::to_string(secondsLeft) +
                    "s | Press N for next level";
            }
            else {
                title =
                    "YOU SAVED CHRISTMAS! FINAL LEVEL COMPLETE!";
            }
        }
        else if (timeLeft <= 0.f) {
            title =
                "TIME'S UP! Santa's orders are late! "
                "Press R to retry";
        }

        window.setTitle(title);



        
window.clear(sf::Color(181, 131, 91));

        for (int x = 0; x <= 800; x += T) {
            sf::RectangleShape line(sf::Vector2f(1.f, 600.f));
            line.setPosition(sf::Vector2f(static_cast<float>(x), 0.f));
            line.setFillColor(sf::Color(150, 190, 200, 100));
            window.draw(line);
        }
        for (int y = 0; y <= 600; y += T) {
            sf::RectangleShape line(sf::Vector2f(800.f, 1.f));
            line.setPosition(sf::Vector2f(0.f, static_cast<float>(y)));
            line.setFillColor(sf::Color(150, 190, 200, 100));
            window.draw(line);
        }


        // =====================================
        // CHRISTMAS WORKSHOP DECORATIONS
        // =====================================

        
        // Wooden baseboard along the workshop wall
        sf::RectangleShape baseboard({800.f, 18.f});
        baseboard.setPosition({0.f, 520.f});
        baseboard.setFillColor(sf::Color(105, 61, 39));
        window.draw(baseboard);

        // Decorative wood grain
        for (int i = 0; i < 13; ++i) {
            sf::RectangleShape grain({800.f, 1.f});
            grain.setPosition({0.f, i * 40.f});
            grain.setFillColor(sf::Color(130, 85, 59, 100));
            window.draw(grain);
        }

        // =====================================
        // INDOOR WORKSHOP FURNITURE
        // =====================================

       
        // =====================================
        // WORKSHOP WINDOW - LEVEL-AWARE POSITION
        // =====================================

        // Move the window away from upper conveyor
        // routes in Levels 3 and 5.
        bool upperRoute = (level == 3 || level == 5);

        float windowX = upperRoute ? 445.f : 315.f;
        float windowW = upperRoute ? 108.f : 145.f;

        sf::RectangleShape windowFrame({windowW, 96.f});
        windowFrame.setPosition({windowX, 85.f});
        windowFrame.setFillColor(sf::Color(95, 55, 35));
        window.draw(windowFrame);

        sf::RectangleShape windowGlass({windowW - 16.f, 80.f});
        windowGlass.setPosition({windowX + 8.f, 93.f});
        windowGlass.setFillColor(sf::Color(95, 145, 175));
        window.draw(windowGlass);

        // Window crossbars
        sf::RectangleShape verticalBar({7.f, 80.f});
        verticalBar.setPosition({
            windowX + windowW / 2.f - 3.5f,
            93.f
        });
        verticalBar.setFillColor(sf::Color(135, 80, 45));
        window.draw(verticalBar);

        sf::RectangleShape horizontalBar({
            windowW - 16.f, 6.f
        });
        horizontalBar.setPosition({
            windowX + 8.f, 130.f
        });
        horizontalBar.setFillColor(sf::Color(135, 80, 45));
        window.draw(horizontalBar);

        // =====================================
        // WINDOW TRIM AND GLASS REFLECTIONS
        // =====================================

        // Outer frame highlights.
        sf::RectangleShape upperTrim({
            windowW + 8.f, 5.f
        });
        upperTrim.setPosition({windowX - 4.f, 81.f});
        upperTrim.setFillColor(
            sf::Color(165, 106, 64)
        );
        window.draw(upperTrim);

        // Wooden windowsill.
        sf::RectangleShape sill({
            windowW + 18.f, 9.f
        });
        sill.setPosition({windowX - 9.f, 180.f});
        sill.setFillColor(
            sf::Color(103, 60, 41)
        );
        window.draw(sill);

        sf::RectangleShape sillHighlight({
            windowW + 18.f, 2.f
        });
        sillHighlight.setPosition({
            windowX - 9.f, 180.f
        });
        sillHighlight.setFillColor(
            sf::Color(207, 144, 80)
        );
        window.draw(sillHighlight);

        // Subtle reflections in the upper panes.
        for (int i = 0; i < 2; ++i) {
            float paneX =
                windowX + 16.f +
                i * (windowW - 16.f) / 2.f;

            sf::RectangleShape reflection({7.f, 27.f});
            reflection.setPosition({paneX, 98.f});
            reflection.setFillColor(
                sf::Color(225, 245, 255, 55)
            );
            window.draw(reflection);
        }




       
        // =====================================
        // TOY SHELVES - AVOID UPPER CONVEYOR ROUTES
        // =====================================

        // Keep shelves shorter where conveyor belts
        // climb into the upper part of the workshop.
        bool compactShelves = (level == 3 || level == 5);

        int toysPerShelf = compactShelves ? 3 : 5;
        float shelfWidth = compactShelves ? 120.f : 200.f;

        for (int shelf = 0; shelf < 2; ++shelf) {
            float shelfY = 117.f + shelf * 42.f;

            sf::RectangleShape plank({shelfWidth, 9.f});
            plank.setPosition({35.f, shelfY});
            plank.setFillColor(sf::Color(100, 55, 30));
            window.draw(plank);

            // Colorful toy boxes
            for (int i = 0; i < toysPerShelf; ++i) {
                sf::RectangleShape toy({22.f, 21.f});
                toy.setPosition({
                    48.f + i * 36.f,
                    shelfY - 22.f
                });

                toy.setFillColor(
                    i % 3 == 0 ? sf::Color(210, 65, 75) :
                    i % 3 == 1 ? sf::Color(70, 145, 100) :
                                 sf::Color(235, 180, 65)
                );

                window.draw(toy);

                // Shaded package side.
                sf::RectangleShape side({5.f, 21.f});
                side.setPosition({
                    65.f + i * 36.f,
                    shelfY - 22.f
                });
                side.setFillColor(
                    sf::Color(42, 45, 55, 75)
                );
                window.draw(side);

                // Decorative gold band.
                sf::RectangleShape band({22.f, 3.f});
                band.setPosition({
                    48.f + i * 36.f,
                    shelfY - 15.f
                });
                band.setFillColor(
                    sf::Color(255, 218, 130, 180)
                );
                window.draw(band);

                // Small inventory emblem.
                sf::CircleShape emblem(2.5f, 12);
                emblem.setPosition({
                    55.f + i * 36.f,
                    shelfY - 10.f
                });
                emblem.setFillColor(
                    sf::Color(255, 242, 200)
                );
                window.draw(emblem);

            }
        }



        // Festive lights strung along the upper wall
        sf::RectangleShape lightWire({760.f, 2.f});
        lightWire.setPosition({20.f, 72.f});
        lightWire.setFillColor(sf::Color(55, 70, 50));
        window.draw(lightWire);

        for (int i = 0; i < 19; ++i) {
            sf::CircleShape bulb(5.f);
            bulb.setPosition({
                27.f + i * 40.f,
                70.f
            });

            bulb.setFillColor(
                i % 3 == 0 ? sf::Color(255, 205, 75) :
                i % 3 == 1 ? sf::Color(240, 75, 85) :
                             sf::Color(85, 220, 125)
            );

            window.draw(bulb);
        }


       
        // =====================================
        // LAYERED CHRISTMAS TREES
        // =====================================

        auto drawChristmasTree = [&](float x, float y) {

            // Ground shadow.
            sf::CircleShape shadow(35.f, 32);
            shadow.setPosition({x - 5.f, y + 82.f});
            shadow.setScale({1.f, 0.24f});
            shadow.setFillColor(
                sf::Color(55, 38, 33, 75)
            );
            window.draw(shadow);

            // Wooden trunk.
            sf::RectangleShape trunk({14.f, 25.f});
            trunk.setPosition({x + 23.f, y + 68.f});
            trunk.setFillColor(sf::Color(102, 58, 36));
            window.draw(trunk);

            sf::RectangleShape trunkLight({4.f, 23.f});
            trunkLight.setPosition({x + 25.f, y + 69.f});
            trunkLight.setFillColor(
                sf::Color(163, 105, 58)
            );
            window.draw(trunkLight);

            for (int i = 2; i >= 0; --i) {
                float top = y + i * 23.f;
                float width = 43.f - i * 3.f;

                // Dark backing creates a layered outline.
                sf::ConvexShape backing(3);
                backing.setPoint(0, {x + 31.f, top - 2.f});
                backing.setPoint(1, {
                    x + 30.f - width, top + 47.f
                });
                backing.setPoint(2, {
                    x + 30.f + width, top + 47.f
                });
                backing.setFillColor(
                    sf::Color(20, 74, 48)
                );
                window.draw(backing);

                // Main foliage.
                sf::ConvexShape foliage(3);
                foliage.setPoint(0, {x + 30.f, top});
                foliage.setPoint(1, {
                    x + 30.f - width + 5.f,
                    top + 43.f
                });
                foliage.setPoint(2, {
                    x + 30.f + width - 5.f,
                    top + 43.f
                });
                foliage.setFillColor(
                    sf::Color(
                        26,
                        static_cast<std::uint8_t>(132 + i * 10),
                        76
                    )
                );
                window.draw(foliage);

                // Shadow on the right side of the tree.
                sf::ConvexShape shade(3);
                shade.setPoint(0, {x + 30.f, top});
                shade.setPoint(1, {
                    x + 31.f, top + 43.f
                });
                shade.setPoint(2, {
                    x + 30.f + width - 5.f,
                    top + 43.f
                });
                shade.setFillColor(
                    sf::Color(12, 69, 52, 115)
                );
                window.draw(shade);

                // Tiny snow-white highlight along a branch.
                sf::RectangleShape glint({12.f, 2.f});
                glint.setPosition({
                    x + 15.f, top + 28.f
                });
                glint.setFillColor(
                    sf::Color(164, 225, 180, 130)
                );
                window.draw(glint);
            }

            // Gold star.
            sf::CircleShape star(8.f, 5);
            star.setOrigin({8.f, 8.f});
            star.setPosition({x + 30.f, y - 7.f});
            star.setFillColor(
                sf::Color(255, 213, 82)
            );
            window.draw(star);

            // Colored baubles with glossy highlights.
            const sf::Vector2f ornaments[] = {
                {19.f, 36.f}, {39.f, 49.f},
                {27.f, 62.f}, {15.f, 75.f},
                {44.f, 80.f}, {29.f, 89.f}
            };

            for (int i = 0; i < 6; ++i) {
                float ox = x + ornaments[i].x;
                float oy = y + ornaments[i].y;

                sf::CircleShape bauble(4.f);
                bauble.setPosition({ox, oy});
                bauble.setFillColor(
                    i % 3 == 0 ? sf::Color(235, 66, 76) :
                    i % 3 == 1 ? sf::Color(255, 211, 76) :
                                 sf::Color(95, 183, 235)
                );
                window.draw(bauble);

                sf::CircleShape shine(1.2f);
                shine.setPosition({ox + 0.7f, oy + 0.7f});
                shine.setFillColor(
                    sf::Color(255, 255, 230, 210)
                );
                window.draw(shine);
            }
        };



        drawChristmasTree(30.f, 395.f);
        drawChristmasTree(705.f, 395.f);

        // =====================================
        // LAUNCHER ELF AT THE LOADING DOCK
        // =====================================

        if (launcherElfHired) {
            const float ex = 102.f;
            const float ey = 260.f;

            sf::RectangleShape body({16.f, 18.f});
            body.setPosition({ex - 8.f, ey});
            body.setFillColor(sf::Color(45, 150, 75));
            window.draw(body);

            sf::CircleShape face(7.f);
            face.setPosition({ex - 7.f, ey - 13.f});
            face.setFillColor(sf::Color(245, 190, 145));
            window.draw(face);

            sf::ConvexShape hat;
            hat.setPointCount(3);
            hat.setPoint(0, {ex, ey - 31.f});
            hat.setPoint(1, {ex - 11.f, ey - 11.f});
            hat.setPoint(2, {ex + 11.f, ey - 11.f});
            hat.setFillColor(sf::Color(30, 135, 65));
            window.draw(hat);

            sf::CircleShape pom(3.f);
            pom.setPosition({ex - 3.f, ey - 34.f});
            pom.setFillColor(sf::Color::White);
            window.draw(pom);
        }


       
        // =====================================
        // CHRISTMAS TOY WAREHOUSE STORAGE
        // =====================================

        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {

                if (!blocked[r][c]) continue;

                float x = static_cast<float>(c * T);
                float y = static_cast<float>(r * T);

                // Slightly different carton colors create
                // varied shelves instead of identical blocks.
                sf::Color boxColor =
                    (r + c) % 3 == 0
                        ? sf::Color(165, 104, 64)
                        : (r + c) % 3 == 1
                            ? sf::Color(185, 125, 75)
                            : sf::Color(150, 98, 66);

                sf::RectangleShape carton({36.f, 36.f});
                carton.setPosition({x + 2.f, y + 2.f});
                carton.setFillColor(boxColor);
                carton.setOutlineThickness(-2.f);
                carton.setOutlineColor(
                    sf::Color(100, 65, 42)
                );
                window.draw(carton);

                // Sealing tape across the carton.
                sf::RectangleShape tape({25.f, 5.f});
                tape.setPosition({x + 8.f, y + 8.f});
                tape.setFillColor(
                    sf::Color(225, 185, 122)
                );
                window.draw(tape);

                // Colored toy-inventory label.
                sf::RectangleShape label({16.f, 11.f});
                label.setPosition({x + 12.f, y + 19.f});
                label.setFillColor(
                    (r + c) % 2 == 0
                        ? sf::Color(195, 65, 75)
                        : sf::Color(65, 145, 100)
                );
                window.draw(label);

                // Small white marking on the label.
                sf::RectangleShape stripe({9.f, 2.f});
                stripe.setPosition({x + 15.f, y + 23.f});
                stripe.setFillColor(
                    sf::Color(240, 230, 205)
                );
                window.draw(stripe);
            }
        }



        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {
                int dir = belts[r][c];
                if (dir < 0) continue;
                float x = static_cast<float>(c * T), y = static_cast<float>(r * T);
               
                // =====================================
                // CONNECTED ANIMATED CONVEYOR TRACKS
                // =====================================

                // Main metal base.
                sf::RectangleShape base({40.f, 40.f});
                base.setPosition({x, y});
                base.setFillColor(sf::Color(72, 82, 95));
                window.draw(base);

                // Find the neighboring conveyor that
                // feeds into this particular tile.
                int entrySide = (dir + 2) % 4;

                for (int d = 0; d < 4; ++d) {
                    int nc = c - dx[d];
                    int nr = r - dy[d];

                    if (inside(nc, nr) &&
                        belts[nr][nc] == d) {

                        entrySide = (d + 2) % 4;
                        break;
                    }
                }

                // Draw a half-track reaching from the
                // center toward one edge of the tile.
                auto drawTrackArm = [&](int side) {
                    bool horizontal = (side == 0 || side == 2);

                    sf::RectangleShape metal(
                        horizontal
                            ? sf::Vector2f(20.f, 28.f)
                            : sf::Vector2f(28.f, 20.f)
                    );

                    float armX =
                        side == 0 ? x + 20.f :
                        side == 2 ? x : x + 6.f;

                    float armY =
                        side == 1 ? y + 20.f :
                        side == 3 ? y : y + 6.f;

                    metal.setPosition({armX, armY});
                    metal.setFillColor(
                        sf::Color(163, 177, 185)
                    );
                    window.draw(metal);

                    sf::RectangleShape movingSurface(
                        horizontal
                            ? sf::Vector2f(20.f, 20.f)
                            : sf::Vector2f(20.f, 20.f)
                    );

                    movingSurface.setPosition(
                        horizontal
                            ? sf::Vector2f(armX, y + 10.f)
                            : sf::Vector2f(x + 10.f, armY)
                    );

                    movingSurface.setFillColor(
                        sf::Color(88, 104, 121)
                    );
                    window.draw(movingSurface);
                };

                drawTrackArm(entrySide);

                if (entrySide != dir) {
                    drawTrackArm(dir);
                }

                // Join the arms seamlessly at the center.
                sf::RectangleShape joint({20.f, 20.f});
                joint.setPosition({x + 10.f, y + 10.f});
                joint.setFillColor(sf::Color(88, 104, 121));
                window.draw(joint);

                // =====================================
                // MOVING CONVEYOR MARKINGS
                // =====================================

                sf::Vector2f center(x + 20.f, y + 20.f);

                sf::Vector2f entry(
                    center.x + dx[entrySide] * 20.f,
                    center.y + dy[entrySide] * 20.f
                );

                sf::Vector2f exitPoint(
                    center.x + dx[dir] * 20.f,
                    center.y + dy[dir] * 20.f
                );

                // The markings travel from the entry
                // side, through the center, to the exit.
                float motion = std::fmod(
                    santaAnimationTime * 28.f, 40.f
                );

                for (int i = 0; i < 4; ++i) {
                    float position =
                        std::fmod(i * 10.f + motion, 40.f);

                    sf::Vector2f point;
                    bool firstHalf = position < 20.f;

                    if (firstHalf) {
                        float t = position / 20.f;

                        point = {
                            entry.x + (center.x - entry.x) * t,
                            entry.y + (center.y - entry.y) * t
                        };
                    }
                    else {
                        float t = (position - 20.f) / 20.f;

                        point = {
                            center.x +
                                (exitPoint.x - center.x) * t,
                            center.y +
                                (exitPoint.y - center.y) * t
                        };
                    }

                    bool onHorizontal =
                        firstHalf
                            ? (entrySide == 0 || entrySide == 2)
                            : (dir == 0 || dir == 2);

                    sf::RectangleShape marker(
                        onHorizontal
                            ? sf::Vector2f(3.f, 15.f)
                            : sf::Vector2f(15.f, 3.f)
                    );

                    marker.setOrigin(
                        onHorizontal
                            ? sf::Vector2f(1.5f, 7.5f)
                            : sf::Vector2f(7.5f, 1.5f)
                    );

                    marker.setPosition(point);
                    marker.setFillColor(
                        sf::Color(180, 205, 220, 165)
                    );
                    window.draw(marker);
                }

                // Direction arrow stays visible above
                // the animated belt markings.
                sf::ConvexShape arrow;
                arrow.setPointCount(3);
                arrow.setPoint(0, {0.f, 0.f});
                arrow.setPoint(1, {13.f, 8.f});
                arrow.setPoint(2, {0.f, 16.f});
                arrow.setOrigin({6.5f, 8.f});
                arrow.setPosition({x + 20.f, y + 20.f});
                arrow.setRotation(sf::degrees(90.f * dir));
                arrow.setFillColor(
                    sf::Color(245, 250, 255)
                );
                window.draw(arrow);





                

                // Orange boxing machine:
                // Packs raw toys into gift boxes.
                if (boxers[r][c]) {

                    sf::RectangleShape machine({30.f, 30.f});
                    machine.setPosition({x + 5.f, y + 5.f});
                    machine.setFillColor(
                        sf::Color(210, 120, 45)
                    );
                    machine.setOutlineThickness(2.f);
                    machine.setOutlineColor(
                        sf::Color(255, 205, 110)
                    );
                    window.draw(machine);

                    // Open cardboard-box symbol
                    sf::RectangleShape boxSymbol({16.f, 12.f});
                    boxSymbol.setPosition({x + 12.f, y + 16.f});
                    boxSymbol.setFillColor(
                        sf::Color(245, 195, 115)
                    );
                    window.draw(boxSymbol);

                    sf::ConvexShape flap;
                    flap.setPointCount(3);
                    flap.setPoint(0, {x + 10.f, y + 16.f});
                    flap.setPoint(1, {x + 20.f, y + 8.f});
                    flap.setPoint(2, {x + 30.f, y + 16.f});
                    flap.setFillColor(
                        sf::Color(255, 220, 155)
                    );
                    window.draw(flap);

                    // Packing press and machine controls.
                    sf::RectangleShape press({20.f, 3.f});
                    press.setPosition({x + 10.f, y + 28.f});
                    press.setFillColor(sf::Color(115, 55, 30));
                    window.draw(press);

                    sf::RectangleShape control({7.f, 5.f});
                    control.setPosition({x + 28.f, y + 7.f});
                    control.setFillColor(sf::Color(55, 70, 68));
                    window.draw(control);

                    sf::CircleShape readyLight(1.8f);
                    readyLight.setPosition({x + 30.f, y + 8.f});
                    readyLight.setFillColor(
                        sf::Color(105, 250, 145)
                    );
                    window.draw(readyLight);

                    // Animated piston while boxing.
                    if (boxingTime > 0.f &&
                        col == c && row == r) {
                        sf::RectangleShape piston({16.f, 3.f});
                        piston.setPosition({
                            x + 12.f,
                            y + 12.f +
                            std::sin(santaAnimationTime * 20.f)
                            * 3.f
                        });
                        piston.setFillColor(
                            sf::Color(255, 245, 190)
                        );
                        window.draw(piston);
                    }


                    // Boxing Elf operates this machine.
                    if (boxingElfHired) {
                        sf::RectangleShape elfBody({14.f, 16.f});
                        elfBody.setPosition({x + 24.f, y - 12.f});
                        elfBody.setFillColor(
                            sf::Color(215, 75, 65)
                        );
                        window.draw(elfBody);

                        sf::CircleShape face(6.f);
                        face.setPosition({x + 25.f, y - 23.f});
                        face.setFillColor(
                            sf::Color(245, 190, 145)
                        );
                        window.draw(face);

                        sf::ConvexShape hat;
                        hat.setPointCount(3);
                        hat.setPoint(0, {x + 31.f, y - 38.f});
                        hat.setPoint(1, {x + 21.f, y - 22.f});
                        hat.setPoint(2, {x + 41.f, y - 22.f});
                        hat.setFillColor(
                            sf::Color(190, 45, 55)
                        );
                        window.draw(hat);
                    }


                }


                
                if (wrappers[r][c]) {
    // Purple wrapping station
    sf::RectangleShape machine({30.f, 30.f});
    machine.setPosition({x + 5.f, y + 5.f});
    machine.setFillColor(sf::Color(125, 65, 175));
    machine.setOutlineThickness(2.f);
    machine.setOutlineColor(sf::Color(220, 165, 255));
    window.draw(machine);

    // Golden wrapping rollers
    for (int i = 0; i < 2; ++i) {
        sf::RectangleShape roller({5.f, 24.f});
        roller.setPosition({
            x + 10.f + i * 15.f, y + 8.f
        });
        roller.setFillColor(sf::Color(255, 205, 75));
        window.draw(roller);
    }

    // Center wrapping mechanism
    sf::CircleShape center(5.f);
    center.setPosition({x + 15.f, y + 15.f});
    center.setFillColor(sf::Color::White);
    window.draw(center);

    // Metallic roller caps and ribbon-feed mechanism.
    for (int i = 0; i < 2; ++i) {
        sf::CircleShape rollerCap(3.5f);
        rollerCap.setPosition({
            x + 9.f + i * 15.f, y + 6.f
        });
        rollerCap.setFillColor(
            sf::Color(255, 235, 140)
        );
        window.draw(rollerCap);
    }

    // Animated ribbon sparkle while wrapping.
    if (wrappingTime > 0.f &&
        col == c && row == r) {

        float pulse =
            (std::sin(santaAnimationTime * 15.f) + 1.f)
            * 0.5f;

        sf::CircleShape sparkle(2.f + pulse * 2.f, 4);
        sparkle.setPosition({
            x + 16.f, y + 12.f
        });
        sparkle.setFillColor(
            sf::Color(255, 245, 150)
        );
        window.draw(sparkle);
    }


    // Show an elf working at the wrapping station
    if (wrappingElfHired) {
        // Elf's body
        sf::CircleShape elfBody(9.f);
        elfBody.setPosition({x + 22.f, y - 17.f});
        elfBody.setFillColor(sf::Color(35, 155, 75));
        window.draw(elfBody);

        // Elf's face
        sf::CircleShape elfFace(7.f);
        elfFace.setPosition({x + 24.f, y - 26.f});
        elfFace.setFillColor(sf::Color(245, 190, 145));
        window.draw(elfFace);

        // Pointed elf hat
        sf::ConvexShape elfHat;
        elfHat.setPointCount(3);
        elfHat.setPoint(0, {x + 31.f, y - 41.f});
        elfHat.setPoint(1, {x + 20.f, y - 25.f});
        elfHat.setPoint(2, {x + 42.f, y - 25.f});
        elfHat.setFillColor(sf::Color(30, 145, 65));
        window.draw(elfHat);

        sf::CircleShape hatPom(3.f);
        hatPom.setPosition({x + 28.f, y - 44.f});
        hatPom.setFillColor(sf::Color::White);
        window.draw(hatPom);
    }

}

                


                if (sorters[r][c]) {
                    // =====================================
                    // AUTOMATED COLOR SCANNING STATION
                    // =====================================

                    // Heavy metal housing
                    sf::RectangleShape housing({34.f, 34.f});
                    housing.setPosition({x + 3.f, y + 3.f});
                    housing.setFillColor(
                        sf::Color(38, 80, 100)
                    );
                    housing.setOutlineThickness(2.f);
                    housing.setOutlineColor(
                        sf::Color(100, 205, 225)
                    );
                    window.draw(housing);

                    // Top scanning window
                    sf::RectangleShape scanner({24.f, 13.f});
                    scanner.setPosition({x + 8.f, y + 7.f});
                    scanner.setFillColor(
                        sf::Color(15, 48, 68)
                    );
                    window.draw(scanner);

                    // Animated scanning line
                    float scanX =
                        x + 10.f +
                        (std::sin(santaAnimationTime * 5.f)
                        + 1.f) * 9.f;

                    sf::RectangleShape scanLine({2.f, 9.f});
                    scanLine.setPosition({scanX, y + 9.f});
                    scanLine.setFillColor(
                        sf::Color(125, 245, 255)
                    );
                    window.draw(scanLine);

                    // Red and blue classification lights
                    sf::CircleShape redLamp(5.f);
                    redLamp.setPosition({x + 9.f, y + 23.f});
                    redLamp.setFillColor(
                        sf::Color(240, 75, 90)
                    );
                    window.draw(redLamp);

                    sf::CircleShape blueLamp(5.f);
                    blueLamp.setPosition({x + 23.f, y + 23.f});
                    blueLamp.setFillColor(
                        sf::Color(75, 155, 245)
                    );
                    window.draw(blueLamp);

                    // Scanner frame with metal corner mounts.
                    for (int i = 0; i < 2; ++i) {
                        sf::RectangleShape bracket({4.f, 5.f});
                        bracket.setPosition({
                            x + (i == 0 ? 5.f : 31.f),
                            y + 8.f
                        });
                        bracket.setFillColor(
                            sf::Color(165, 205, 215)
                        );
                        window.draw(bracket);
                    }

                    // Identification strip under the scanner.
                    sf::RectangleShape panel({18.f, 2.f});
                    panel.setPosition({x + 11.f, y + 35.f});
                    panel.setFillColor(
                        sf::Color(100, 225, 230)
                    );
                    window.draw(panel);


                    // Status lamp, red when sabotaged
                    bool damaged =
                        brokenC == c && brokenR == r;

                    sf::CircleShape statusLamp(3.f);
                    statusLamp.setPosition({x + 17.f, y + 1.f});
                    statusLamp.setFillColor(
                        damaged
                            ? sf::Color(255, 65, 65)
                            : sf::Color(115, 255, 150)
                    );
                    window.draw(statusLamp);
                }



            }
        }
        // Red and blue storage bins at the top of the workshop.
        for (int color = 0; color < 2; ++color) {
            float bx = color == 0 ? 560.f : 680.f;
            sf::RectangleShape bin(sf::Vector2f(95.f, 55.f));
            bin.setPosition(sf::Vector2f(bx, 65.f));
            bin.setFillColor(color == 0 ? sf::Color(130, 45, 55)
                                         : sf::Color(40, 75, 140));
            window.draw(bin);

            // Storage bin frame and top rim.
            bin.setOutlineThickness(2.f);
            bin.setOutlineColor(
                sf::Color(185, 195, 200)
            );
            // Redraw the outline on top.
            window.draw(bin);

            sf::RectangleShape topRim({95.f, 6.f});
            topRim.setPosition({bx, 65.f});
            topRim.setFillColor(
                color == 0
                    ? sf::Color(185, 85, 95)
                    : sf::Color(85, 130, 195)
            );
            window.draw(topRim);

            // Small dark inventory label plate.
            sf::RectangleShape labelPlate({32.f, 11.f});
            labelPlate.setPosition({bx + 57.f, 103.f});
            labelPlate.setFillColor(
                sf::Color(30, 43, 55)
            );
            window.draw(labelPlate);

            // Metallic side reinforcement.
            sf::RectangleShape sideRail({5.f, 44.f});
            sideRail.setPosition({bx + 2.f, 71.f});
            sideRail.setFillColor(
                sf::Color(170, 180, 190, 155)
            );
            window.draw(sideRail);


            int count = color == 0 ? redStored : blueStored;
            for (int i = 0; i < std::min(count, 6); ++i) {
                sf::RectangleShape gift(sf::Vector2f(15.f, 15.f));
                gift.setPosition(sf::Vector2f(bx + 8.f + (i % 3) * 27.f,
                                            70.f + (i / 3) * 23.f));
                gift.setFillColor(color == 0 ? sf::Color(240, 90, 100)
                                              : sf::Color(90, 160, 250));
                window.draw(gift);
            }
        }

        // Storage-bin inventory and click instructions.
        if (fontLoaded) {
            sf::Text redCount(
                hudFont,
                "RED: " + std::to_string(redStored),
                13
            );
            redCount.setPosition({565.f, 122.f});
            redCount.setFillColor(sf::Color(105, 30, 40));
            window.draw(redCount);

            sf::Text blueCount(
                hudFont,
                "BLUE: " + std::to_string(blueStored),
                13
            );
            blueCount.setPosition({685.f, 122.f});
            blueCount.setFillColor(sf::Color(30, 60, 120));
            window.draw(blueCount);
        }


        sf::RectangleShape goal(sf::Vector2f(38.f, 38.f));
        goal.setPosition(sf::Vector2f(GX * T + 1.f, GY * T + 1.f));
        goal.setFillColor(sf::Color(50, 180, 85));
        window.draw(goal);
        sf::RectangleShape start(sf::Vector2f(38.f, 38.f));
        start.setPosition(sf::Vector2f(SX * T + 1.f, SY * T + 1.f));
        start.setFillColor(sf::Color::Transparent);
        start.setOutlineThickness(-3.f);
        start.setOutlineColor(sf::Color(40, 120, 230));
        window.draw(start);


        // =====================================
        // SANTA - FACTORY MANAGER
        // =====================================
        {
            const float sx = 744.f;
            const float sy = 255.f;

            bool celebrating = santaCheerTime > 0.f ||
                (delivered >= levelGoal);

            float bob = std::sin(santaAnimationTime * 2.5f)
                * 2.f;

            // Soft shadow beneath Santa.
            sf::CircleShape santaShadow(29.f, 32);
            santaShadow.setScale({1.f, 0.23f});
            santaShadow.setPosition({
                sx - 29.f, sy + 34.f
            });
            santaShadow.setFillColor(
                sf::Color(45, 35, 35, 65)
            );
            window.draw(santaShadow);


            // Santa's boots
            for (int i = 0; i < 2; ++i) {
                sf::RectangleShape boot({14.f, 11.f});
                boot.setPosition({
                    sx - 17.f + i * 20.f,
                    sy + 28.f + bob
                });
                boot.setFillColor(sf::Color(45, 40, 45));
                window.draw(boot);
            }

            // Red coat
            sf::RectangleShape coat({39.f, 40.f});
            coat.setPosition({sx - 19.f, sy - 8.f + bob});
            coat.setFillColor(sf::Color(205, 45, 55));
            window.draw(coat);

            // White coat trim
            sf::RectangleShape trim({7.f, 40.f});
            trim.setPosition({sx - 3.f, sy - 8.f + bob});
            trim.setFillColor(sf::Color(250, 245, 235));
            window.draw(trim);

            // Black belt and gold buckle
            sf::RectangleShape belt({39.f, 7.f});
            belt.setPosition({sx - 19.f, sy + 15.f + bob});
            belt.setFillColor(sf::Color(35, 35, 40));
            window.draw(belt);

            sf::RectangleShape buckle({9.f, 9.f});
            buckle.setPosition({sx - 4.f, sy + 14.f + bob});
            buckle.setFillColor(sf::Color(245, 200, 65));
            window.draw(buckle);

            // =====================================
            // SANTA'S COAT DETAILS
            // =====================================

            // Gold buttons on the white coat trim.
            for (int i = 0; i < 2; ++i) {
                sf::CircleShape button(2.1f);
                button.setPosition({
                    sx - 0.5f,
                    sy - 1.f + i * 12.f + bob
                });
                button.setFillColor(
                    sf::Color(230, 185, 75)
                );
                window.draw(button);
            }

            // Soft red coat highlights.
            sf::RectangleShape leftSeam({2.f, 28.f});
            leftSeam.setPosition({
                sx - 16.f, sy - 5.f + bob
            });
            leftSeam.setFillColor(
                sf::Color(240, 90, 90, 135)
            );
            window.draw(leftSeam);

            sf::RectangleShape rightSeam({2.f, 28.f});
            rightSeam.setPosition({
                sx + 13.f, sy - 5.f + bob
            });
            rightSeam.setFillColor(
                sf::Color(120, 25, 40, 140)
            );
            window.draw(rightSeam);

            // White fur trim along the bottom of the coat.
            sf::RectangleShape lowerTrim({39.f, 5.f});
            lowerTrim.setPosition({
                sx - 19.f, sy + 27.f + bob
            });
            lowerTrim.setFillColor(
                sf::Color(240, 239, 230)
            );
            window.draw(lowerTrim);


            // Arms - raised when celebrating
            float armY = celebrating ? sy - 22.f : sy + 1.f;

            for (int i = 0; i < 2; ++i) {
                sf::RectangleShape arm({12.f, 22.f});
                arm.setPosition({
                    sx - 29.f + i * 47.f,
                    armY + bob
                });
                arm.setFillColor(sf::Color(205, 45, 55));
                window.draw(arm);

                sf::CircleShape glove(6.f);
                glove.setPosition({
                    sx - 29.f + i * 47.f,
                    armY + (celebrating ? -7.f : 17.f) + bob
                });
                glove.setFillColor(sf::Color(45, 45, 50));
                window.draw(glove);
            }

            // Head
            sf::CircleShape head(18.f);
            head.setPosition({sx - 18.f, sy - 41.f + bob});
            head.setFillColor(sf::Color(250, 195, 155));
            window.draw(head);

            // White beard
            sf::CircleShape beard(16.f);
            beard.setPosition({sx - 16.f, sy - 24.f + bob});
            beard.setFillColor(sf::Color(250, 250, 245));
            window.draw(beard);

            // Face
            for (int i = 0; i < 2; ++i) {
                sf::CircleShape eye(2.f);
                eye.setPosition({
                    sx - 9.f + i * 15.f,
                    sy - 29.f + bob
                });
                eye.setFillColor(sf::Color(35, 35, 40));
                window.draw(eye);
            }

            sf::CircleShape nose(4.f);
            nose.setPosition({sx - 4.f, sy - 23.f + bob});
            nose.setFillColor(sf::Color(235, 120, 115));
            window.draw(nose);

            // =====================================
            // SANTA'S EXPRESSIVE FACE
            // =====================================

            // White eyebrows above the eyes.
            for (int i = 0; i < 2; ++i) {
                sf::RectangleShape eyebrow({9.f, 3.f});
                eyebrow.setPosition({
                    sx - 12.f + i * 15.f,
                    sy - 34.f + bob
                });
                eyebrow.setFillColor(
                    sf::Color(250, 248, 240)
                );
                window.draw(eyebrow);
            }

            // Two overlapping mustache halves.
            for (int i = 0; i < 2; ++i) {
                sf::CircleShape mustache(7.f, 20);
                mustache.setScale({1.2f, 0.55f});
                mustache.setPosition({
                    sx - 14.f + i * 13.f,
                    sy - 18.f + bob
                });
                mustache.setFillColor(
                    sf::Color(255, 255, 250)
                );
                window.draw(mustache);
            }

            // Gentle cheek highlights.
            for (int i = 0; i < 2; ++i) {
                sf::CircleShape cheek(3.f);
                cheek.setPosition({
                    sx - 15.f + i * 23.f,
                    sy - 24.f + bob
                });
                cheek.setFillColor(
                    sf::Color(240, 145, 135, 135)
                );
                window.draw(cheek);
            }


            // Santa's hat
            sf::ConvexShape hat;
            hat.setPointCount(3);
            hat.setPoint(0, {sx + 2.f, sy - 67.f + bob});
            hat.setPoint(1, {sx - 20.f, sy - 37.f + bob});
            hat.setPoint(2, {sx + 20.f, sy - 37.f + bob});
            hat.setFillColor(sf::Color(195, 35, 50));
            window.draw(hat);

            // Slightly darker fold in Santa's hat.
            sf::ConvexShape hatFold;
            hatFold.setPointCount(3);
            hatFold.setPoint(0, {
                sx + 2.f, sy - 67.f + bob
            });
            hatFold.setPoint(1, {
                sx + 20.f, sy - 37.f + bob
            });
            hatFold.setPoint(2, {
                sx + 9.f, sy - 37.f + bob
            });
            hatFold.setFillColor(
                sf::Color(145, 28, 43, 120)
            );
            window.draw(hatFold);



            sf::RectangleShape hatTrim({42.f, 7.f});
            hatTrim.setPosition({
                sx - 21.f, sy - 43.f + bob
            });
            hatTrim.setFillColor(sf::Color::White);
            window.draw(hatTrim);

            sf::CircleShape pom(6.f);
            pom.setPosition({sx - 4.f, sy - 73.f + bob});
            pom.setFillColor(sf::Color::White);
            window.draw(pom);

            // Golden celebration sparkles
            if (celebrating) {
                for (int i = 0; i < 4; ++i) {
                    float sparkleX = sx - 33.f + i * 21.f;
                    float sparkleY = sy - 80.f +
                        std::sin(santaAnimationTime * 8.f + i)
                        * 5.f;

                    sf::CircleShape sparkle(4.f, 4);
                    sparkle.setPosition({sparkleX, sparkleY});
                    sparkle.setFillColor(
                        sf::Color(255, 215, 70)
                    );
                    window.draw(sparkle);
                }
            }
        }


        // =====================================
        // POLISHED ELF DETAILS
        // =====================================
        // faceX / faceY = center of the elf's face.
        // bodyY = upper edge of the elf's uniform.

        auto drawElfDetails = [&](
            float faceX, float faceY,
            float bodyY, bool small
        ) {
            float scale = small ? 0.75f : 1.f;

            // Bright eyes with tiny highlights.
            for (int side = -1; side <= 1; side += 2) {
                float eyeX =
                    faceX + side * 3.5f * scale;

                sf::CircleShape eye(1.8f * scale);
                eye.setPosition({
                    eyeX - 1.8f * scale,
                    faceY - 1.8f * scale
                });
                eye.setFillColor(sf::Color(45, 38, 42));
                window.draw(eye);

                sf::CircleShape shine(0.7f * scale);
                shine.setPosition({
                    eyeX - 0.7f * scale,
                    faceY - 1.3f * scale
                });
                shine.setFillColor(sf::Color::White);
                window.draw(shine);
            }

            // Rosy cheeks.
            for (int side = -1; side <= 1; side += 2) {
                sf::CircleShape cheek(2.1f * scale);
                cheek.setPosition({
                    faceX + side * 6.f * scale
                        - 2.1f * scale,
                    faceY + 2.f * scale
                });
                cheek.setFillColor(
                    sf::Color(225, 125, 120, 190)
                );
                window.draw(cheek);
            }

            // Small smiling mouth.
            sf::RectangleShape smile({
                4.f * scale, 1.5f * scale
            });
            smile.setPosition({
                faceX - 2.f * scale,
                faceY + 4.5f * scale
            });
            smile.setFillColor(
                sf::Color(145, 72, 70)
            );
            window.draw(smile);

            // Black uniform belt and gold buckle.
            sf::RectangleShape belt({
                15.f * scale, 3.f * scale
            });
            belt.setPosition({
                faceX - 7.5f * scale,
                bodyY + 12.f * scale
            });
            belt.setFillColor(
                sf::Color(48, 55, 58)
            );
            window.draw(belt);

            sf::RectangleShape buckle({
                4.f * scale, 4.f * scale
            });
            buckle.setPosition({
                faceX - 2.f * scale,
                bodyY + 11.5f * scale
            });
            buckle.setFillColor(
                sf::Color(255, 205, 75)
            );
            window.draw(buckle);
        };


       
        // =====================================
        // SANTA-STYLE WORKSHOP ELF MAKEOVER
        // =====================================
        // 0 Launcher, 1 Boxer, 2 Wrapper,
        // 3 Sorter, 4 Repair

        auto drawElfAccessories = [&](
            float cx, float bodyY,
            bool small, int role
        ) {
            float s = small ? 0.85f : 1.f;

            sf::Color coat =
                role == 1 || role == 4
                    ? sf::Color(184, 52, 67)
                    : sf::Color(35, 139, 80);

            sf::Color highlight =
                role == 1 || role == 4
                    ? sf::Color(235, 92, 93)
                    : sf::Color(78, 188, 112);

            sf::Color accent =
                role == 0 ? sf::Color(255, 207, 78) :
                role == 1 ? sf::Color(255, 160, 75) :
                role == 2 ? sf::Color(200, 125, 245) :
                role == 3 ? sf::Color(95, 225, 240) :
                            sf::Color(255, 205, 90);

            sf::Color skin(247, 193, 152);
            sf::Color cream(255, 244, 218);

            auto part = [&](float x, float y,
                            float w, float h,
                            sf::Color color) {
                sf::RectangleShape shape({w*s, h*s});
                shape.setPosition({cx + x*s, bodyY + y*s});
                shape.setFillColor(color);
                window.draw(shape);
            };

            auto circle = [&](float x, float y,
                              float rad, sf::Color color) {
                sf::CircleShape shape(rad*s, 24);
                shape.setPosition({cx + x*s, bodyY + y*s});
                shape.setFillColor(color);
                window.draw(shape);
            };

            // Boots and darker trousers.
            for (int side : {-1, 1}) {
                part(side * 5.f - 3.f, 17.f,
                     6.f, 10.f, sf::Color(58, 60, 92));

                part(side * 5.f - 4.f, 24.f,
                     9.f, 5.f, sf::Color(38, 43, 56));

                part(side * 5.f - 3.f, 25.f,
                     4.f, 1.5f, sf::Color(117, 126, 145));
            }

            // Rounded sleeves and gloves.
            circle(-13.f, 6.f, 5.f, coat);
            circle(3.f, 6.f, 5.f, coat);

            circle(-14.f, 11.f, 3.f, skin);
            circle(8.f, 11.f, 3.f, skin);

            // Coat: dark outline with illuminated front.
            part(-10.f, 0.f, 20.f, 21.f,
                 sf::Color(55, 65, 58));
            part(-9.f, 0.f, 18.f, 19.f, coat);
            part(-8.f, 2.f, 4.f, 15.f, highlight);

            // Fur at the collar and lower coat hem.
            part(-9.f, 0.f, 18.f, 3.f, cream);
            part(-9.f, 17.f, 18.f, 3.f, cream);

            // Front trim.
            part(-1.f, 3.f, 2.f, 14.f, cream);

            // Uniform belt and golden buckle.
            part(-9.f, 13.f, 18.f, 3.f,
                 sf::Color(41, 45, 53));
            part(-2.5f, 12.5f, 5.f, 4.f,
                 sf::Color(255, 207, 83));

            // Job-specific badge.
            part(4.f, 5.f, 4.f, 5.f, accent);

            // Distinctive pointed ears.
            circle(-11.f, -8.f, 3.f, skin);
            circle(5.f, -8.f, 3.f, skin);

            // Soft shaded head, similar to Santa.
            circle(-9.f, -17.f, 9.f,
                   sf::Color(211, 145, 111));
            circle(-9.f, -19.f, 8.5f, skin);

            // Gentle cheek highlights.
            circle(-7.f, -8.f, 2.5f,
                   sf::Color(241, 137, 131, 180));
            circle(3.f, -8.f, 2.5f,
                   sf::Color(241, 137, 131, 180));

            // Eyes and bright highlights.
            for (int side : {-1, 1}) {
                float ex = side * 3.5f;

                circle(ex - 1.5f, -13.f, 1.6f,
                       sf::Color(38, 47, 50));

                circle(ex - 0.8f, -13.f, 0.55f,
                       sf::Color::White);
            }

            // Tiny nose and smile.
            circle(-1.5f, -9.5f, 1.5f,
                   sf::Color(224, 148, 119));

            part(-2.f, -5.f, 4.f, 1.2f,
                 sf::Color(153, 80, 77));

            // Pointed hat with a darker side fold.
            sf::ConvexShape hat(3);
            hat.setPoint(0, {cx, bodyY - 34.f*s});
            hat.setPoint(1, {
                cx - 13.f*s, bodyY - 17.f*s
            });
            hat.setPoint(2, {
                cx + 13.f*s, bodyY - 17.f*s
            });
            hat.setFillColor(coat);
            window.draw(hat);

            sf::ConvexShape fold(3);
            fold.setPoint(0, {cx, bodyY - 34.f*s});
            fold.setPoint(1, {
                cx + 13.f*s, bodyY - 17.f*s
            });
            fold.setPoint(2, {
                cx + 3.f*s, bodyY - 17.f*s
            });
            fold.setFillColor(
                sf::Color(30, 40, 43, 65)
            );
            window.draw(fold);

            // White hat brim.
            part(-12.f, -18.f, 24.f, 4.f, cream);

            // Gold hat bell.
            circle(-3.f, -38.f, 3.f,
                   sf::Color(255, 212, 90));
        };




        // =====================================
        // REPAIR ELF CHARACTER
        // =====================================
        if (repairElfHired) {
            float swing =
                std::sin(repairElfWalkPhase) * 3.f;

            // Legs
            for (int i = 0; i < 2; ++i) {
                sf::RectangleShape leg({5.f, 11.f});
                leg.setPosition({
                    repairElfX - 7.f + i * 10.f,
                    repairElfY + 5.f +
                        (i == 0 ? swing : -swing)
                });
                leg.setFillColor(sf::Color(70, 65, 110));
                window.draw(leg);
            }

            // Red uniform
            sf::RectangleShape body({18.f, 19.f});
            body.setPosition({
                repairElfX - 9.f, repairElfY - 11.f
            });
            body.setFillColor(sf::Color(205, 55, 65));
            window.draw(body);

            // Face
            sf::CircleShape face(8.f);
            face.setPosition({
                repairElfX - 8.f, repairElfY - 23.f
            });
            face.setFillColor(sf::Color(245, 190, 145));
            window.draw(face);

            // Hat
            sf::ConvexShape hat;
            hat.setPointCount(3);
            hat.setPoint(0, {
                repairElfX, repairElfY - 39.f
            });
            hat.setPoint(1, {
                repairElfX - 11.f, repairElfY - 20.f
            });
            hat.setPoint(2, {
                repairElfX + 11.f, repairElfY - 20.f
            });
            hat.setFillColor(sf::Color(190, 35, 55));
            window.draw(hat);
        }

        // =====================================
        // GREMLIN AT THE BROKEN MACHINE
        // =====================================
        if (brokenC >= 0) {
            float gx = brokenC * T + 20.f;
            float gy = brokenR * T - 15.f;

            float bob =
                std::sin(gremlinTimer * 5.f) * 3.f;

            // Green body
            sf::CircleShape gremlinBody(12.f);
            gremlinBody.setPosition({
                gx - 12.f, gy + bob
            });
            gremlinBody.setFillColor(
                sf::Color(95, 170, 55)
            );
            window.draw(gremlinBody);

            // Dark side shading gives the gremlin depth.
            sf::CircleShape darkSide(9.f, 24);
            darkSide.setPosition({
                gx - 11.f, gy + 5.f + bob
            });
            darkSide.setFillColor(
                sf::Color(34, 91, 46, 110)
            );
            window.draw(darkSide);

            // Bright green face and belly highlight.
            sf::CircleShape belly(7.f, 24);
            belly.setPosition({
                gx - 7.f, gy + 7.f + bob
            });
            belly.setFillColor(
                sf::Color(153, 204, 88, 180)
            );
            window.draw(belly);

            // A tiny shadow above the cheek area.
            sf::CircleShape forehead(6.f, 24);
            forehead.setPosition({
                gx - 6.f, gy + 1.f + bob
            });
            forehead.setScale({1.f, 0.35f});
            forehead.setFillColor(
                sf::Color(37, 102, 46, 100)
            );
            window.draw(forehead);


            // Pointy ears
            sf::ConvexShape ears;
            ears.setPointCount(3);
            ears.setPoint(0, {gx - 18.f, gy - 9.f + bob});
            ears.setPoint(1, {gx, gy + 3.f + bob});
            ears.setPoint(2, {gx + 18.f, gy - 9.f + bob});
            ears.setFillColor(sf::Color(65, 135, 45));
            window.draw(ears);

            // Eyes
            for (int i = 0; i < 2; ++i) {
                sf::CircleShape eye(2.5f);
                eye.setPosition({
                    gx - 6.f + i * 10.f,
                    gy + 7.f + bob
                });
                eye.setFillColor(sf::Color::White);
                window.draw(eye);
            }

            // =====================================
            // GREMLIN FACE AND MISCHIEVOUS DETAILS
            // =====================================

            // Dark pupils looking sideways.
            for (int i = 0; i < 2; ++i) {
                sf::CircleShape pupil(1.3f);
                pupil.setPosition({
                    gx - 4.f + i*10.f,
                    gy + 8.f + bob
                });
                pupil.setFillColor(
                    sf::Color(35, 43, 35)
                );
                window.draw(pupil);
            }

            // Angry slanted eyebrows.
            for (int i = 0; i < 2; ++i) {
                sf::RectangleShape brow({8.f, 2.f});
                brow.setPosition({
                    gx - 9.f + i*10.f,
                    gy + 3.f + bob
                });
                brow.setRotation(
                    sf::degrees(i == 0 ? 18.f : -18.f)
                );
                brow.setFillColor(
                    sf::Color(42, 85, 38)
                );
                window.draw(brow);
            }

            // Dark grinning mouth.
            sf::RectangleShape mouth({12.f, 5.f});
            mouth.setPosition({
                gx - 6.f, gy + 16.f + bob
            });
            mouth.setFillColor(
                sf::Color(68, 45, 53)
            );
            window.draw(mouth);

            // Two tiny teeth.
            for (int i = 0; i < 2; ++i) {
                sf::ConvexShape tooth(3);
                float tx = gx - 4.f + i*7.f;

                tooth.setPoint(0, {
                    tx, gy + 16.f + bob
                });
                tooth.setPoint(1, {
                    tx + 3.f, gy + 16.f + bob
                });
                tooth.setPoint(2, {
                    tx + 1.5f, gy + 20.f + bob
                });
                tooth.setFillColor(
                    sf::Color(250, 245, 215)
                );
                window.draw(tooth);
            }

            // Tiny clawed hands.
            for (int side : {-1, 1}) {
                sf::CircleShape hand(4.f, 12);
                hand.setPosition({
                    gx + side*13.f - 4.f,
                    gy + 15.f + bob
                });
                hand.setFillColor(
                    sf::Color(75, 145, 45)
                );
                window.draw(hand);

                sf::ConvexShape claw(3);
                float hx = gx + side*14.f;
                claw.setPoint(0, {
                    hx - 2.f, gy + 20.f + bob
                });
                claw.setPoint(1, {
                    hx + 2.f, gy + 20.f + bob
                });
                claw.setPoint(2, {
                    hx + side*4.f,
                    gy + 25.f + bob
                });
                claw.setFillColor(
                    sf::Color(225, 220, 175)
                );
                window.draw(claw);
            }

            // A small yellow sabotage spark.
            float sparkPulse =
                2.f + std::sin(santaAnimationTime * 12.f);

            sf::CircleShape sabotageSpark(
                sparkPulse, 4
            );
            sabotageSpark.setPosition({
                gx + 12.f,
                gy - 11.f + bob
            });
            sabotageSpark.setFillColor(
                sf::Color(255, 211, 78)
            );
            window.draw(sabotageSpark);


            // Warning light over the damaged machine
            sf::CircleShape warning(5.f);
            warning.setPosition({
                brokenC * T + 30.f,
                brokenR * T + 4.f
            });
            warning.setFillColor(
                sf::Color(255, 65, 65)
            );
            window.draw(warning);
        }

        // =====================================
        // ANIMATED MANUAL REPAIR INDICATOR
        // =====================================

        if (brokenC >= 0 && manualRepairTime > 0.f) {
            float cx = brokenC * T + 20.f;
            float cy = brokenR * T + 20.f;

            float motion =
                std::sin(santaAnimationTime * 13.f);

            // Progress bar background
            sf::RectangleShape barBack({54.f, 8.f});
            barBack.setPosition({
                cx - 27.f, cy - 48.f
            });
            barBack.setFillColor(
                sf::Color(45, 45, 50)
            );
            window.draw(barBack);

            // Four-second repair progress
            float completed =
                std::clamp(
                    1.f - manualRepairTime / 4.f,
                    0.f, 1.f
                );

            sf::RectangleShape barFill({
                52.f * completed, 6.f
            });
            barFill.setPosition({
                cx - 26.f, cy - 47.f
            });
            barFill.setFillColor(
                sf::Color(105, 240, 145)
            );
            window.draw(barFill);

            // Animated wrench handle
            sf::RectangleShape wrench({5.f, 25.f});
            wrench.setOrigin({2.5f, 20.f});
            wrench.setPosition({
                cx + 4.f,
                cy - 12.f + motion * 3.f
            });
            wrench.setRotation(
                sf::degrees(motion * 28.f)
            );
            wrench.setFillColor(
                sf::Color(220, 225, 230)
            );
            window.draw(wrench);

            // Wrench head
            sf::CircleShape wrenchHead(7.f, 8);
            wrenchHead.setOrigin({7.f, 7.f});
            wrenchHead.setPosition({
                cx + 4.f,
                cy - 31.f + motion * 3.f
            });
            wrenchHead.setFillColor(
                sf::Color(165, 180, 195)
            );
            window.draw(wrenchHead);

            // Little golden repair sparks
            for (int i = 0; i < 3; ++i) {
                sf::CircleShape spark(2.5f, 4);
                spark.setPosition({
                    cx - 14.f + i * 15.f,
                    cy - 29.f +
                    std::sin(santaAnimationTime * 10.f + i)
                        * 5.f
                });
                spark.setFillColor(
                    sf::Color(255, 210, 80)
                );
                window.draw(spark);
            }
        }



        // Draw the sorting elf
        if (sortingElfHired) {
            // Walking legs
            float swing = std::sin(sortingElfWalkPhase) * 3.f;

            sf::RectangleShape leftLeg({5.f, 11.f});
            leftLeg.setPosition({
                sortingElfX - 7.f,
                sortingElfY + 5.f + swing
            });
            leftLeg.setFillColor(sf::Color(65, 75, 125));
            window.draw(leftLeg);

            sf::RectangleShape rightLeg({5.f, 11.f});
            rightLeg.setPosition({
                sortingElfX + 2.f,
                sortingElfY + 5.f - swing
            });
            rightLeg.setFillColor(sf::Color(65, 75, 125));
            window.draw(rightLeg);

            // Green uniform
            sf::RectangleShape elfBody({18.f, 19.f});
            elfBody.setPosition({
                sortingElfX - 9.f, sortingElfY - 11.f
            });
            elfBody.setFillColor(sf::Color(35, 155, 75));
            window.draw(elfBody);

            // Face
            sf::CircleShape elfFace(8.f);
            elfFace.setPosition({
                sortingElfX - 8.f, sortingElfY - 23.f
            });
            elfFace.setFillColor(sf::Color(245, 190, 145));
            window.draw(elfFace);

            // Pointed hat
            sf::ConvexShape elfHat;
            elfHat.setPointCount(3);
            elfHat.setPoint(0, {
                sortingElfX, sortingElfY - 39.f
            });
            elfHat.setPoint(1, {
                sortingElfX - 11.f, sortingElfY - 20.f
            });
            elfHat.setPoint(2, {
                sortingElfX + 11.f, sortingElfY - 20.f
            });
            elfHat.setFillColor(sf::Color(30, 135, 55));
            window.draw(elfHat);

            // Hat pom-pom
            sf::CircleShape pom(3.f);
            pom.setPosition({
                sortingElfX - 3.f, sortingElfY - 42.f
            });
            pom.setFillColor(sf::Color::White);
            window.draw(pom);
        }


        // =====================================
        // FINAL ELF FACE AND UNIFORM DETAILS
        // =====================================

        // Launcher Elf
        if (launcherElfHired) {
            drawElfDetails(
                102.f, 254.f, 260.f, true
            );
        }

        // Boxing Elf
        if (boxingElfHired) {
            for (int r = 0; r < ROWS; ++r) {
                for (int c = 0; c < COLS; ++c) {
                    if (boxers[r][c]) {
                        float x = c * T;
                        float y = r * T;

                        drawElfDetails(
                            x + 31.f,
                            y - 17.f,
                            y - 12.f,
                            true
                        );
                    }
                }
            }
        }

        // Wrapping Elf
        if (wrappingElfHired) {
            for (int r = 0; r < ROWS; ++r) {
                for (int c = 0; c < COLS; ++c) {
                    if (wrappers[r][c]) {
                        float x = c * T;
                        float y = r * T;

                        drawElfDetails(
                            x + 31.f,
                            y - 19.f,
                            y - 17.f,
                            true
                        );
                    }
                }
            }
        }

        // Sorting Elf
        if (sortingElfHired) {
            drawElfDetails(
                sortingElfX,
                sortingElfY - 15.f,
                sortingElfY - 11.f,
                false
            );
        }

        // Repair Elf
        if (repairElfHired) {
            drawElfDetails(
                repairElfX,
                repairElfY - 15.f,
                repairElfY - 11.f,
                false
            );
        }


        // =====================================
        // ELF UNIFORMS AND ROLE ACCESSORIES
        // =====================================

        if (launcherElfHired) {
            drawElfAccessories(
                102.f, 260.f, true, 0
            );
        }

        if (boxingElfHired) {
            for (int r = 0; r < ROWS; ++r) {
                for (int c = 0; c < COLS; ++c) {
                    if (boxers[r][c]) {
                        float y = r * T;
                        float x = c * T;

                        drawElfAccessories(
                            x + 31.f,
                            y - 12.f,
                            true, 1
                        );
                    }
                }
            }
        }

        if (wrappingElfHired) {
            for (int r = 0; r < ROWS; ++r) {
                for (int c = 0; c < COLS; ++c) {
                    if (wrappers[r][c]) {
                        float y = r * T;
                        float x = c * T;

                        drawElfAccessories(
                            x + 31.f,
                            y - 17.f,
                            true, 2
                        );
                    }
                }
            }
        }

        if (sortingElfHired) {
            drawElfAccessories(
                sortingElfX,
                sortingElfY - 11.f,
                false, 3
            );
        }

        if (repairElfHired) {
            drawElfAccessories(
                repairElfX,
                repairElfY - 11.f,
                false, 4
            );
        }


        float px = (col + (traveling ? (nextCol - col) * progress : 0.f)) * T + 8.f;
        float py = (row + (traveling ? (nextRow - row) * progress : 0.f)) * T + 8.f;
       
        
        // =====================================
        // TOY PRODUCTION VISUALS
        // =====================================

       
        // =====================================
        // POLISHED TOY AND GIFT GRAPHICS
        // =====================================

        // Small reusable shape helpers.
        auto toyRect = [&](float x, float y,
                           float w, float h,
                           sf::Color color) {
            sf::RectangleShape shape({w, h});
            shape.setPosition({x, y});
            shape.setFillColor(color);
            window.draw(shape);
        };

        auto toyCircle = [&](float x, float y,
                             float radius,
                             sf::Color color) {
            sf::CircleShape shape(radius, 24);
            shape.setPosition({x, y});
            shape.setFillColor(color);
            window.draw(shape);
        };

        // The current item casts a soft shadow.
        toyRect(px + 2.f, py + 25.f,
                25.f, 5.f,
                sf::Color(35, 35, 40, 65));

        if (!boxed) {

            if (presentType == 0) {
                // =====================================
                // DETAILED TEDDY BEAR
                // =====================================

                const sf::Color brown(176, 105, 62);
                const sf::Color lightBrown(220, 158, 103);
                const sf::Color darkBrown(82, 53, 49);

                // Little rounded feet.
                toyCircle(px + 2.f, py + 19.f,
                          5.f, brown);
                toyCircle(px + 17.f, py + 19.f,
                          5.f, brown);

                // Arms.
                toyCircle(px - 1.f, py + 13.f,
                          5.f, brown);
                toyCircle(px + 19.f, py + 13.f,
                          5.f, brown);

                // Body.
                toyCircle(px + 3.f, py + 10.f,
                          10.f, brown);

                // Tummy.
                toyCircle(px + 8.f, py + 15.f,
                          5.f, lightBrown);

                // Ears and their inner details.
                for (int i = 0; i < 2; ++i) {
                    float earX = px + 2.f + i * 16.f;

                    toyCircle(earX, py,
                              5.f, brown);
                    toyCircle(earX + 1.5f, py + 1.5f,
                              3.f, lightBrown);
                }

                // Head.
                toyCircle(px + 3.f, py + 3.f,
                          10.f, brown);

                // Muzzle.
                toyCircle(px + 8.f, py + 12.f,
                          5.f, lightBrown);

                // Eyes and highlights.
                for (int i = 0; i < 2; ++i) {
                    float eyeX = px + 8.f + i * 9.f;

                    toyCircle(eyeX, py + 9.f,
                              1.8f, darkBrown);
                    toyCircle(eyeX + .5f, py + 9.2f,
                              .55f, sf::Color::White);
                }

                // Nose and smile.
                toyCircle(px + 11.f, py + 14.f,
                          2.f, darkBrown);

                toyRect(px + 12.5f, py + 17.5f,
                        1.f, 3.f, darkBrown);

                // Little red bow tie.
                sf::ConvexShape leftBow(3);
                leftBow.setPoint(0, {px + 12.f, py + 20.f});
                leftBow.setPoint(1, {px + 5.f, py + 17.f});
                leftBow.setPoint(2, {px + 5.f, py + 23.f});
                leftBow.setFillColor(sf::Color(205, 52, 72));
                window.draw(leftBow);

                sf::ConvexShape rightBow(3);
                rightBow.setPoint(0, {px + 13.f, py + 20.f});
                rightBow.setPoint(1, {px + 20.f, py + 17.f});
                rightBow.setPoint(2, {px + 20.f, py + 23.f});
                rightBow.setFillColor(sf::Color(205, 52, 72));
                window.draw(rightBow);

                toyCircle(px + 11.f, py + 18.5f,
                          2.f, sf::Color(245, 190, 75));
            }
            else {
                // =====================================
                // DETAILED BLUE TOY CAR
                // =====================================

                // Dark undercarriage.
                toyRect(px, py + 16.f, 26.f, 9.f,
                        sf::Color(38, 57, 86));

                // Lower car body.
                toyRect(px - 1.f, py + 13.f,
                        28.f, 11.f,
                        sf::Color(35, 103, 205));

                // Hood and body highlights.
                toyRect(px + 1.f, py + 14.f,
                        24.f, 3.f,
                        sf::Color(95, 175, 250));

                // Roof.
                toyRect(px + 6.f, py + 6.f,
                        15.f, 9.f,
                        sf::Color(40, 125, 215));

                // Two windows with a divider.
                toyRect(px + 8.f, py + 8.f,
                        5.f, 5.f,
                        sf::Color(165, 225, 245));

                toyRect(px + 15.f, py + 8.f,
                        4.f, 5.f,
                        sf::Color(165, 225, 245));

                // Wheels and silver hubs.
                for (int i = 0; i < 2; ++i) {
                    float wheelX = px + 3.f + i * 17.f;

                    toyCircle(wheelX, py + 20.f,
                              4.5f, sf::Color(30, 38, 48));

                    toyCircle(wheelX + 2.f, py + 22.f,
                              2.f, sf::Color(180, 195, 207));
                }

                // Headlight and taillight.
                toyRect(px + 24.f, py + 17.f,
                        3.f, 4.f,
                        sf::Color(255, 225, 135));

                toyRect(px - 1.f, py + 17.f,
                        2.f, 4.f,
                        sf::Color(240, 85, 90));

                // Tiny door handle.
                toyRect(px + 13.f, py + 17.f,
                        4.f, 1.5f,
                        sf::Color(220, 232, 242));
            }
        }
        else {
            // =====================================
            // DETAILED BOXED AND WRAPPED PRESENTS
            // =====================================

            sf::Color boxColor =
                presentType == 0
                    ? sf::Color(207, 51, 73)
                    : sf::Color(48, 105, 215);

            sf::Color lighterColor =
                presentType == 0
                    ? sf::Color(245, 112, 128)
                    : sf::Color(115, 170, 250);

            // Package shadow.
            toyRect(px + 3.f, py + 5.f,
                    25.f, 24.f,
                    sf::Color(65, 55, 65, 100));

            // Main gift box.
            toyRect(px, py + 2.f,
                    24.f, 23.f, boxColor);

            // Side shading.
            toyRect(px + 20.f, py + 5.f,
                    4.f, 20.f,
                    sf::Color(45, 45, 80, 95));

            // Bright lid.
            toyRect(px - 2.f, py,
                    28.f, 6.f, lighterColor);

            // Tiny cardboard seam.
            toyRect(px + 2.f, py + 8.f,
                    20.f, 2.f,
                    sf::Color(255, 255, 255, 45));

            if (wrapped) {
                const sf::Color ribbon(255, 209, 77);

                // Ribbon wrapping around both sides.
                toyRect(px + 9.f, py,
                        6.f, 25.f, ribbon);

                toyRect(px, py + 10.f,
                        24.f, 6.f, ribbon);

                // Narrow ribbon highlights.
                toyRect(px + 10.f, py,
                        1.5f, 25.f,
                        sf::Color(255, 245, 160));

                // Bow loops.
                toyCircle(px + 4.f, py - 6.f,
                          5.f, ribbon);

                toyCircle(px + 12.f, py - 6.f,
                          5.f, ribbon);

                // Centers of the bow loops.
                toyCircle(px + 7.f, py - 3.f,
                          2.f, lighterColor);

                toyCircle(px + 15.f, py - 3.f,
                          2.f, lighterColor);

                // Knot.
                toyCircle(px + 10.f, py - 2.f,
                          3.f, sf::Color(255, 240, 130));
            }
        }



        // =====================================
        // ANIMATED TOY DISPENSER AT THE LOADING DOCK
        // =====================================

        {
            const float lx = SX * T;
            const float ly = SY * T;

            bool toyAtDock =
                col == SX && row == SY && !traveling;

            // Floor shadow beneath the dispenser.
            sf::RectangleShape machineShadow({40.f, 9.f});
            machineShadow.setPosition({lx + 2.f, ly + 33.f});
            machineShadow.setFillColor(
                sf::Color(35, 40, 43, 85)
            );
            window.draw(machineShadow);

            // Reinforced metal casing.
            sf::RectangleShape dispenser({37.f, 35.f});
            dispenser.setPosition({lx + 1.f, ly + 2.f});
            dispenser.setFillColor(
                sf::Color(53, 99, 119)
            );
            dispenser.setOutlineThickness(2.f);
            dispenser.setOutlineColor(
                sf::Color(170, 205, 215)
            );
            window.draw(dispenser);

            // Top metal housing.
            sf::RectangleShape roof({35.f, 7.f});
            roof.setPosition({lx + 2.f, ly + 2.f});
            roof.setFillColor(
                sf::Color(110, 159, 172)
            );
            window.draw(roof);

            // Dark delivery chute.
            sf::RectangleShape chute({22.f, 19.f});
            chute.setPosition({lx + 15.f, ly + 12.f});
            chute.setFillColor(
                sf::Color(24, 47, 65)
            );
            window.draw(chute);

            // Shutter covers the waiting toy.
            if (toyAtDock) {
                sf::RectangleShape shutter({24.f, 23.f});
                shutter.setPosition({lx + 13.f, ly + 10.f});
                shutter.setFillColor(
                    sf::Color(73, 126, 151)
                );
                window.draw(shutter);

                for (int i = 0; i < 3; ++i) {
                    sf::RectangleShape rib({19.f, 2.f});
                    rib.setPosition({
                        lx + 15.f, ly + 14.f + i * 6.f
                    });
                    rib.setFillColor(
                        sf::Color(155, 190, 200)
                    );
                    window.draw(rib);
                }
            }

            // Metallic outlet facing the next belt.
            sf::RectangleShape exitRail({5.f, 31.f});
            exitRail.setPosition({lx + 35.f, ly + 5.f});
            exitRail.setFillColor(
                sf::Color(185, 192, 198)
            );
            window.draw(exitRail);

            // Launcher status light.
            sf::CircleShape status(3.5f);
            status.setPosition({lx + 5.f, ly + 12.f});

            status.setFillColor(
                waitingForLaunch
                    ? sf::Color(255, 195, 60)
                    : productionRunning
                        ? sf::Color(75, 240, 130)
                        : sf::Color(230, 90, 90)
            );
            window.draw(status);

            // Tiny animated operating light.
            if (productionRunning &&
                !waitingForLaunch) {

                float pulse =
                    (std::sin(santaAnimationTime * 9.f)
                     + 1.f) * 0.5f;

                sf::RectangleShape activity({4.f, 11.f});
                activity.setPosition({lx + 6.f, ly + 21.f});
                activity.setFillColor(
                    sf::Color(
                        80,
                        static_cast<std::uint8_t>(
                            135.f + pulse * 100.f
                        ),
                        160
                    )
                );
                window.draw(activity);
            }
        }



        sf::RectangleShape progressBack(sf::Vector2f(200.f, 14.f));
        progressBack.setPosition(sf::Vector2f(300.f, 25.f));
        progressBack.setFillColor(sf::Color(80, 100, 110));
        window.draw(progressBack);
        sf::RectangleShape progressFill(sf::Vector2f(200.f * delivered / levelGoal, 14.f));
        progressFill.setPosition(sf::Vector2f(300.f, 25.f));
        progressFill.setFillColor(sf::Color(40, 180, 80));
        window.draw(progressFill);


        
        // =====================================
        // NEW FACTORY DASHBOARD
        // =====================================

        const sf::Color darkGreen(28, 69, 59);
        const sf::Color lightCream(245, 228, 190);
        const sf::Color gold(245, 198, 89);

        // Right sidebar
        sf::RectangleShape sidebar({200.f, 700.f});
        sidebar.setPosition({800.f, 0.f});
        sidebar.setFillColor(darkGreen);
        window.draw(sidebar);

        // Sidebar divider
        sf::RectangleShape separator({3.f, 700.f});
        separator.setPosition({800.f, 0.f});
        separator.setFillColor(gold);
        window.draw(separator);

        // Bottom shortcut strip
        sf::RectangleShape bottomPanel({800.f, 100.f});
        bottomPanel.setPosition({0.f, 600.f});
        bottomPanel.setFillColor(darkGreen);
        window.draw(bottomPanel);

        // =====================================
        // POLISHED DASHBOARD FRAME
        // =====================================

        // Slightly brighter header in the sidebar.
        sf::RectangleShape sidebarHeader({200.f, 105.f});
        sidebarHeader.setPosition({800.f, 0.f});
        sidebarHeader.setFillColor(
            sf::Color(37, 89, 73)
        );
        window.draw(sidebarHeader);

        // Dark lower edge beneath the header.
        sf::RectangleShape headerEdge({200.f, 3.f});
        headerEdge.setPosition({800.f, 103.f});
        headerEdge.setFillColor(
            sf::Color(15, 49, 43)
        );
        window.draw(headerEdge);

        // Subtle gold section separator.
        sf::RectangleShape headerGold({176.f, 1.5f});
        headerGold.setPosition({812.f, 108.f});
        headerGold.setFillColor(
            sf::Color(238, 190, 85, 125)
        );
        window.draw(headerGold);

        // Bottom panel: lighter top edge and shadow.
        sf::RectangleShape barShadow({800.f, 5.f});
        barShadow.setPosition({0.f, 600.f});
        barShadow.setFillColor(
            sf::Color(14, 42, 39)
        );
        window.draw(barShadow);

        sf::RectangleShape barTrim({800.f, 2.f});
        barTrim.setPosition({0.f, 600.f});
        barTrim.setFillColor(
            sf::Color(228, 185, 85)
        );
        window.draw(barTrim);


        // Helpers for drawing the new interface
        auto hudText = [&](const std::string& message,
                           unsigned int size,
                           float x, float y,
                           sf::Color color = sf::Color::White) {
            if (!fontLoaded) return;

            sf::Text text(hudFont, message, size);
            text.setPosition({x, y});
            text.setFillColor(color);
            window.draw(text);
        };

        auto hudButton = [&](const std::string& label,
                             float y, float height,
                             bool selected = false,
                             bool enabled = true) {

            sf::RectangleShape button({176.f, height});
            button.setPosition({812.f, y});

            button.setFillColor(
                !enabled ? sf::Color(65, 85, 80) :
                selected ? sf::Color(184, 131, 56) :
                           sf::Color(54, 109, 89)
            );

            button.setOutlineThickness(1.f);
            button.setOutlineColor(
                selected ? gold :
                           sf::Color(110, 157, 130)
            );

            window.draw(button);

            // A small highlight gives each button depth.
            sf::RectangleShape buttonGloss({
                174.f, 2.f
            });
            buttonGloss.setPosition({
                813.f, y + 1.f
            });
            buttonGloss.setFillColor(
                enabled
                    ? sf::Color(225, 255, 230, 85)
                    : sf::Color(180, 185, 180, 30)
            );
            window.draw(buttonGloss);

            if (selected) {
                sf::RectangleShape selectionMark({
                    4.f, height - 4.f
                });
                selectionMark.setPosition({
                    814.f, y + 2.f
                });
                selectionMark.setFillColor(gold);
                window.draw(selectionMark);
            }

            hudText(
                label, 14,
                823.f, y + (height - 20.f) / 2.f,
                enabled ? sf::Color::White :
                          sf::Color(165, 180, 170)
            );
        };

        // =====================================
        // TOP OF SIDEBAR: FACTORY STATUS
        // =====================================

        hudText("SANTA'S FACTORY", 18, 811.f, 12.f, gold);

        hudText(
            "LEVEL " + std::to_string(level) +
            "/" + std::to_string(MAX_LEVEL),
            15, 814.f, 47.f
        );

        hudText(
            "TIME: " +
            std::to_string(
                static_cast<int>(timeLeft + 0.999f)
            ) + "s",
            15, 900.f, 47.f
        );

        hudText(
            "MONEY: $" + std::to_string(money),
            17, 814.f, 78.f, gold
        );

        // =====================================
        // SANTA'S CURRENT ORDER
        // =====================================

        sf::RectangleShape orderPanel({176.f, 88.f});
        orderPanel.setPosition({812.f, 115.f});
        orderPanel.setFillColor(lightCream);
        window.draw(orderPanel);

        // Framed parchment-style order card.
        sf::RectangleShape orderBorder({174.f, 86.f});
        orderBorder.setPosition({813.f, 116.f});
        orderBorder.setFillColor(
            sf::Color::Transparent
        );
        orderBorder.setOutlineThickness(1.f);
        orderBorder.setOutlineColor(
            sf::Color(160, 117, 64)
        );
        window.draw(orderBorder);

        sf::RectangleShape orderAccent({4.f, 84.f});
        orderAccent.setPosition({813.f, 117.f});
        orderAccent.setFillColor(
            sf::Color(216, 169, 77)
        );
        window.draw(orderAccent);


        hudText(
            "SANTA NEEDS:",
            14, 824.f, 122.f,
            sf::Color(75, 58, 45)
        );

        sf::Color orderColor =
            orderType == 0
                ? sf::Color(215, 60, 75)
                : sf::Color(55, 115, 225);

        sf::RectangleShape colorSwatch({29.f, 29.f});
        colorSwatch.setPosition({824.f, 150.f});
        colorSwatch.setFillColor(orderColor);
        window.draw(colorSwatch);

        hudText(
            orderType == 0 ? "RED" : "BLUE",
            21, 863.f, 148.f, orderColor
        );

        hudText(
            "GIFTS: " + std::to_string(delivered) +
            "/" + std::to_string(levelGoal),
            13, 824.f, 181.f,
            sf::Color(65, 65, 60)
        );

        // =====================================
        // RUSH ORDER STATUS
        // =====================================

        sf::RectangleShape rushStatus({176.f, 52.f});
        rushStatus.setPosition({812.f, 211.f});
        rushStatus.setFillColor(
            rushActive
                ? sf::Color(157, 49, 54)
                : sf::Color(46, 83, 75)
        );
        window.draw(rushStatus);

        if (rushActive) {
            hudText(
                "RUSH ORDER!",
                16, 823.f, 216.f, gold
            );

            hudText(
                std::string(
                    rushOrderType == 0 ? "RED" : "BLUE"
                ) + "  " +
                std::to_string(
                    static_cast<int>(rushTimeLeft + 0.999f)
                ) + "s  +$60",
                13, 823.f, 240.f
            );
        }
        else {
            hudText(
                "RUSH ORDERS",
                14, 823.f, 216.f, gold
            );
            hudText(
                "No active rush",
                13, 823.f, 240.f
            );
        }

      
        // =====================================
        // FACTORY UPGRADES
        // =====================================

        hudText(
            "FACTORY UPGRADES",
            15, 813.f, 268.f, gold
        );

        int beltCost = 80 * (conveyorUpgrade + 1);
        int repairCost = 70 * (repairUpgrade + 1);
        int sortCost = 70 * (sorterUpgrade + 1);

        std::string beltLabel =
            conveyorUpgrade >= 2
                ? "B | BELT: MAX"
                : "B | BELT LV " +
                  std::to_string(conveyorUpgrade) +
                  ">"+ std::to_string(conveyorUpgrade + 1) +
                  " $" + std::to_string(beltCost);

        std::string repairLabel =
            !repairElfHired
                ? "Z | REPAIR: LOCKED"
                : repairUpgrade >= 2
                    ? "Z | REPAIR: MAX"
                    : "Z | REPAIR LV " +
                      std::to_string(repairUpgrade) +
                      ">" + std::to_string(repairUpgrade + 1) +
                      " $" + std::to_string(repairCost);

        std::string sortLabel =
            !sortingElfHired
                ? "T | SORT: LOCKED"
                : sorterUpgrade >= 2
                    ? "T | SORT: MAX"
                    : "T | SORT LV " +
                      std::to_string(sorterUpgrade) +
                      ">" + std::to_string(sorterUpgrade + 1) +
                      " $" + std::to_string(sortCost);

        hudButton(
            beltLabel,
            295.f, 31.f, false,
            conveyorUpgrade < 2 && money >= beltCost
        );

        hudButton(
            repairLabel,
            332.f, 31.f, false,
            repairElfHired &&
            repairUpgrade < 2 &&
            money >= repairCost
        );

        hudButton(
            sortLabel,
            369.f, 31.f, false,
            sortingElfHired &&
            sorterUpgrade < 2 &&
            money >= sortCost
        );



        // =====================================
        // SHOP AND PRODUCTION
        // =====================================

        hudButton(
            "P  |  +3 BELTS  $60",
            466.f, 37.f,
            false, money >= BELT_PACK_COST
        );

        hudButton(
            productionRunning
                ? "PRODUCTION RUNNING"
                : "G  |  START FACTORY",
            517.f, 42.f,
            !productionRunning,
            !productionRunning
        );

       
        // Click anywhere in this section to open the shop.
        hudText(
            "H: EMPLOYEE SHOP",
            14, 813.f, 577.f, gold
        );

        hudButton(
            "H  |  HIRE WORKERS",
            601.f, 45.f,
            hiringMenuOpen, true
        );

        int employeeCount =
            static_cast<int>(launcherElfHired) +
            static_cast<int>(boxingElfHired) +
            static_cast<int>(wrappingElfHired) +
            static_cast<int>(sortingElfHired) +
            static_cast<int>(repairElfHired);

        hudText(
            "EMPLOYEES: " +
            std::to_string(employeeCount) + "/5",
            13, 813.f, 656.f, gold
        );

        hudText(
            "Click or press H",
            12, 813.f, 677.f
        );



        // =====================================
        // BOTTOM BAR: QUICK INFORMATION
        // =====================================

        hudText(
            "BELTS AVAILABLE: " + std::to_string(partsLeft) +
            "     SCORE: " + std::to_string(score),
            18, 20.f, 607.f, gold
        );


       
        // =====================================
        // SINGLE CONTEXTUAL FACTORY STATUS
        // =====================================

        std::string factoryStatus = "PRODUCTION ACTIVE";
        sf::Color factoryStatusColor(130, 235, 170);

       
if (delivered >= levelGoal) {
    factoryStatus =
        level == MAX_LEVEL
            ? "CHRISTMAS SAVED!"
            : "LEVEL COMPLETE - N: NEXT";

    factoryStatusColor =
        sf::Color(125, 255, 175);
}
else if (timeLeft <= 0.f) {
    factoryStatus = "TIME'S UP - R: RETRY";
    factoryStatusColor =
        sf::Color(255, 145, 145);
}
else if (!productionRunning) {
    factoryStatus = "BUILD MODE";
    factoryStatusColor = gold;
}
else if (brokenC >= 0) {

            factoryStatus = manualRepairTime > 0.f
                ? "REPAIR IN PROGRESS..."
                : "BROKEN MACHINE - X";
            factoryStatusColor = sf::Color(255, 135, 135);
        }
        else if (waitingForBox) {
            factoryStatus = "SPACE: BOX TOY";
            factoryStatusColor = gold;
        }
        else if (waitingForWrap) {
            factoryStatus = "SPACE: ADD RIBBON";
            factoryStatusColor = gold;
        }
        else if (waitingForSort && !sortingElfHired) {
            factoryStatus = "A: APPROVE / S: STORE";
            factoryStatusColor = gold;
        }
        else if (waitingForLaunch) {
            factoryStatus = launcherElfHired
                ? "ELF PREPARING TOY"
                : "E: SEND NEXT TOY";
            factoryStatusColor = gold;
        }

        
        // Clear, highlighted message for the player's
        // next action. The rest of the controls live
        // in the pause menu.

        sf::RectangleShape statusPanel({310.f, 36.f});
        statusPanel.setPosition({405.f, 601.f});
        statusPanel.setFillColor(
            sf::Color(37, 86, 72)
        );
        statusPanel.setOutlineThickness(1.f);
        statusPanel.setOutlineColor(
            sf::Color(125, 170, 140)
        );
        window.draw(statusPanel);

        hudText(
            factoryStatus,
            16, 417.f, 608.f,
            factoryStatusColor
        );


        // =====================================
        // MUSIC CONTROL BUTTON
        // =====================================

        sf::RectangleShape musicButton({204.f, 36.f});
        musicButton.setPosition({570.f, 657.f});

        musicButton.setFillColor(
            musicMuted
                ? sf::Color(105, 70, 70)
                : sf::Color(54, 109, 89)
        );

        musicButton.setOutlineThickness(1.f);
        musicButton.setOutlineColor(
            sf::Color(245, 198, 89)
        );

        window.draw(musicButton);

        hudText(
            musicMuted
                ? "M  |  MUSIC: OFF"
                : "M  |  MUSIC: ON",
            15, 587.f, 664.f,
            sf::Color::White
        );
        // Build Mode instruction banner
        // Santa's special rush order banner


        // =====================================
        // SANTA'S SHORT RUSH ANNOUNCEMENT
        // =====================================

        if (rushAnnouncementTime > 0.f &&
            gameStarted && !gamePaused) {

            // A quick animated entrance.
            float elapsed =
                1.6f - rushAnnouncementTime;

            float slide =
                std::max(0.f, 1.f - elapsed * 5.f)
                * 90.f;

            sf::RectangleShape banner({380.f, 76.f});
            banner.setPosition({
                210.f, 95.f - slide
            });

            banner.setFillColor(
                sf::Color(155, 45, 55, 235)
            );
            banner.setOutlineThickness(3.f);
            banner.setOutlineColor(
                sf::Color(255, 215, 95)
            );

            window.draw(banner);

            hudText(
                "SANTA'S RUSH ORDER!",
                22, 227.f, 104.f - slide,
                sf::Color(255, 225, 130)
            );

            hudText(
                std::string("NEEDED: ") +
                    (rushOrderType == 0 ? "RED" : "BLUE") +
                    "  |  BONUS: $60",
                16, 228.f, 137.f - slide,
                sf::Color::White
            );
        }


        // =====================================
        // FIVE-EMPLOYEE HIRING MENU
        // =====================================

        if (hiringMenuOpen && gameStarted && !gamePaused) {

            // Darken only the factory play area.
            sf::RectangleShape shopBackdrop({800.f, 600.f});
            shopBackdrop.setFillColor(
                sf::Color(10, 25, 25, 180)
            );
            window.draw(shopBackdrop);

            sf::RectangleShape shopPanel({480.f, 355.f});
            shopPanel.setPosition({160.f, 150.f});
            shopPanel.setFillColor(
                sf::Color(27, 73, 62)
            );
            shopPanel.setOutlineThickness(3.f);
            shopPanel.setOutlineColor(gold);
            window.draw(shopPanel);

            hudText(
                "SANTA'S EMPLOYEE SHOP",
                23, 190.f, 160.f, gold
            );

            hudText(
                "Choose 1-5 or click an employee | H to close",
                13, 190.f, 188.f
            );

            const std::string names[5] = {
                "1  LAUNCHER ELF",
                "2  BOXING ELF",
                "3  WRAPPING ELF",
                "4  SORTING ELF",
                "5  REPAIR ELF"
            };

            const int costs[5] = {
                LAUNCHER_ELF_COST,
                BOXING_ELF_COST,
                ELF_COST,
                SORTING_ELF_COST,
                REPAIR_ELF_COST
            };

            const bool hired[5] = {
                launcherElfHired,
                boxingElfHired,
                wrappingElfHired,
                sortingElfHired,
                repairElfHired
            };

            for (int i = 0; i < 5; ++i) {
                float y = 210.f + i * 52.f;
                bool affordable = money >= costs[i];

                sf::RectangleShape option({420.f, 40.f});
                option.setPosition({190.f, y});

                option.setFillColor(
                    hired[i] ? sf::Color(60, 80, 75) :
                    affordable ? sf::Color(55, 125, 95) :
                                 sf::Color(80, 70, 68)
                );

                option.setOutlineThickness(1.f);
                option.setOutlineColor(
                    sf::Color(140, 175, 145)
                );

                window.draw(option);

                hudText(names[i], 16, 204.f, y + 8.f);

                hudText(
                    hired[i]
                        ? "HIRED"
                        : "$" + std::to_string(costs[i]),
                    15, 525.f, y + 8.f,
                    hired[i] ? sf::Color(140, 235, 165) :
                    affordable ? gold :
                                 sf::Color(190, 190, 190)
                );
            }
        }


        // =========================================
        // LEVEL COMPLETE / TIME'S UP SCREEN
        // =========================================

        bool levelWon = delivered >= levelGoal;
        bool levelLost = timeLeft <= 0.f && !levelWon;

        if (levelWon || levelLost) {
            // Darken the center of the factory
            sf::RectangleShape resultPanel(
                sf::Vector2f(520.f, 150.f)
            );
            resultPanel.setPosition(
                sf::Vector2f(140.f, 220.f)
            );
            resultPanel.setFillColor(
                sf::Color(20, 45, 45, 240)
            );
            resultPanel.setOutlineThickness(3.f);
            resultPanel.setOutlineColor(
                levelWon
                    ? sf::Color(70, 220, 120)
                    : sf::Color(235, 75, 85)
            );
            window.draw(resultPanel);

            if (fontLoaded) {
                std::string resultMessage =
                    levelWon ? "LEVEL COMPLETE!"
                             : "TIME'S UP!";

                if (levelWon && level == MAX_LEVEL) {
                    resultMessage = "CHRISTMAS SAVED!";
                }

                sf::Text result(
                    hudFont, resultMessage, 30
                );
                result.setStyle(sf::Text::Bold);
                result.setPosition(
                    sf::Vector2f(180.f, 245.f)
                );
                result.setFillColor(
                    levelWon
                        ? sf::Color(100, 240, 150)
                        : sf::Color(255, 110, 110)
                );
                window.draw(result);

                std::string instruction;

                if (levelLost) {
                    instruction = "Press R to retry";
                }
                else if (level < MAX_LEVEL) {
                    instruction =
                        "Press N for the next level";
                }
                else {
                    instruction =
                        "You completed Santa's Factory!";
                }

                sf::Text nextText(
                    hudFont, instruction, 21
                );
                nextText.setPosition(
                    sf::Vector2f(180.f, 305.f)
                );
                nextText.setFillColor(
                    sf::Color::White
                );
                window.draw(nextText);
            }
        }


       
        // =====================================
        // CLEAN START MENU AND PAUSE GUIDE
        // =====================================

        if (!gameStarted || gamePaused) {

            sf::RectangleShape overlay({1000.f, 700.f});
            overlay.setFillColor(
                sf::Color(12, 35, 40, 225)
            );
            window.draw(overlay);

            sf::RectangleShape panel({630.f, 505.f});
            panel.setPosition({85.f, 92.f});
            panel.setFillColor(sf::Color(25, 77, 63));
            panel.setOutlineThickness(3.f);
            panel.setOutlineColor(sf::Color(245, 198, 89));
            window.draw(panel);

            // =====================================
            // DECORATIVE MENU FINISHING
            // =====================================

            // Warm illuminated header.
            sf::RectangleShape menuHeader({
                622.f, 83.f
            });
            menuHeader.setPosition({89.f, 96.f});
            menuHeader.setFillColor(
                sf::Color(43, 106, 82)
            );
            window.draw(menuHeader);

            // Gold rule beneath the title area.
            sf::RectangleShape goldRule({
                610.f, 2.f
            });
            goldRule.setPosition({95.f, 195.f});
            goldRule.setFillColor(
                sf::Color(245, 198, 89)
            );
            window.draw(goldRule);

            // Decorative winter foliage in the
            // upper-right corner of the menu.
            for (int i = 0; i < 3; ++i) {
                sf::CircleShape leaf(9.f, 16);
                leaf.setScale({1.f, 0.5f});
                leaf.setPosition({
                    640.f + i * 13.f,
                    125.f + (i % 2) * 11.f
                });
                leaf.setFillColor(
                    sf::Color(78, 164, 110)
                );
                window.draw(leaf);
            }

            // Holly berries.
            for (int i = 0; i < 3; ++i) {
                sf::CircleShape berry(4.f);
                berry.setPosition({
                    663.f + (i % 2) * 8.f,
                    142.f + (i / 2) * 8.f
                });
                berry.setFillColor(
                    sf::Color(225, 75, 80)
                );
                window.draw(berry);
            }

            // Lower decorative rule.
            sf::RectangleShape lowerRule({
                604.f, 1.f
            });
            lowerRule.setPosition({98.f, 583.f});
            lowerRule.setFillColor(
                sf::Color(245, 198, 89, 100)
            );
            window.draw(lowerRule);


            if (fontLoaded) {

                const sf::Color menuGold(245, 207, 115);
                const sf::Color menuWhite(235, 243, 238);
                const sf::Color menuMint(145, 240, 180);

                auto menuText = [&](const std::string& value,
                                    unsigned size,
                                    float x, float y,
                                    sf::Color color) {

                    sf::Text text(hudFont, value, size);
                    text.setPosition({x, y});
                    text.setFillColor(color);
                    window.draw(text);
                };

                menuText(
                    gamePaused
                        ? "FACTORY PAUSED"
                        : "SANTA'S FACTORY",
                    34, 120.f, 112.f, menuGold
                );

                menuText(
                    gamePaused
                        ? "The elves are taking a break."
                        : "Save Christmas, one gift at a time!",
                    17, 120.f, 165.f, menuWhite
                );

                menuText(
                    gamePaused
                        ? "Press ESC to resume"
                        : "Press ENTER to start",
                    22, 120.f, 202.f, menuMint
                );

                // Draw a neatly separated help section.
                auto helpSection = [&](float x, float y,
                                       const std::string& heading,
                                       const std::string& lines) {

                    sf::RectangleShape card({285.f, 124.f});
                    card.setPosition({x, y});
                    card.setFillColor(
                        sf::Color(37, 101, 82)
                    );
                    card.setOutlineThickness(1.f);
                    card.setOutlineColor(
                        sf::Color(90, 150, 120)
                    );
                    window.draw(card);

                    menuText(
                        heading, 17,
                        x + 13.f, y + 9.f,
                        menuGold
                    );

                    menuText(
                        lines, 14,
                        x + 13.f, y + 39.f,
                        menuWhite
                    );
                };

                helpSection(
                    105.f, 258.f,
                    "PRODUCTION",
                    "G - Start production\n"
                    "E - Send next toy\n"
                    "SPACE - Box / wrap toy\n"
                    "A / S - Approve / store"
                );

                helpSection(
                    410.f, 258.f,
                    "WORKSHOP",
                    "Left Click - Place / rotate belts\n"
                    "Right Click - Remove belts\n"
                    "P - Buy 3 more belts\n"
                    "Click crates - Clear for $80"
                );

                helpSection(
                    105.f, 396.f,
                    "UPGRADES",
                    "B - Faster conveyor belts\n"
                    "Z - Faster Repair Elf\n"
                    "T - Faster Sorting Elf\n"
                    "Elf upgrades require hiring"
                );

                helpSection(
                    410.f, 396.f,
                    "EMPLOYEES & GAME",
                    "H - Hire elves (choose 1-5)\n"
                    "X - Repair broken machines\n"
                    "M - Toggle music\n"
                    "R - Retry | N - Next level"
                );
            }
        }





        window.display();
    }
    return 0;
}