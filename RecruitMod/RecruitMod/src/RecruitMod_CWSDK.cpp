// RecruitMod_CWSDK.cpp  — compatible avec ton CWSDK "cube.h" + headers Ghidra

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <cwchar>

#include <cwsdk/cube.h>                    // ⚠️ le bon agrégateur, en minuscules

#include "RecruitMod.h"

namespace RecruitMod {

    namespace {
        // === utilitaires robustes au layout / noms ===

        // Détecte si T a un membre .parent_owner
        template <typename T>
        auto test_parent_owner(int) -> decltype((void)std::declval<T>().parent_owner, std::true_type{}) { return {}; }
        template <typename> std::false_type test_parent_owner(...) { return {}; }

        // Détecte si T a un membre .entity_data.parent_owner
        template <typename T>
        auto test_entity_data_parent_owner(int) -> decltype((void)std::declval<T>().entity_data.parent_owner, std::true_type{}) { return {}; }
        template <typename> std::false_type test_entity_data_parent_owner(...) { return {}; }

        inline int64_t GetParentOwner(const cube::Creature* c) {
            if constexpr (decltype(test_parent_owner<cube::Creature>(0))::value)
                return c->parent_owner;
            else if constexpr (decltype(test_entity_data_parent_owner<cube::Creature>(0))::value)
                return c->entity_data.parent_owner;
            else
                return -1;
        }

        // Détecte hostility_flags à la racine
        template <typename T>
        auto test_hostility_root(int) -> decltype((void)std::declval<T>().hostility_flags, std::true_type{}) { return {}; }
        template <typename> std::false_type test_hostility_root(...) { return {}; }

        // Détecte hostility_flags sous entity_data
        template <typename T>
        auto test_hostility_in_entity(int) -> decltype((void)std::declval<T>().entity_data.hostility_flags, std::true_type{}) { return {}; }
        template <typename> std::false_type test_hostility_in_entity(...) { return {}; }

        inline uint32_t GetHostility(const cube::Creature* c) {
            if constexpr (decltype(test_hostility_root<cube::Creature>(0))::value)
                return c->hostility_flags;
            else if constexpr (decltype(test_hostility_in_entity<cube::Creature>(0))::value)
                return c->entity_data.hostility_flags;
            else
                return 0u;
        }

        // Copie les entités du monde vers un std::map quelle que soit la casse du membre (Entities/entities)
        template <typename WorldT>
        void SnapshotCreatures(WorldT& world, std::map<uint64_t, cube::Creature*>& out) {
            // Tentative : world.Entities
            if constexpr (requires(WorldT w) { w.Entities.CopyToSTDMap(out); }) {
                world.Entities.CopyToSTDMap(out);
            }
            else if constexpr (requires(WorldT w) { w.entities.CopyToSTDMap(out); }) {
                world.entities.CopyToSTDMap(out);
            }
            else {
                // Pas de membre direct exposé : on laisse la map vide.
            }
        }

        // Chat helper
        inline void Chat(const std::wstring& msg, const Color& col = Color::White()) {
            if (auto* gc = cube::GetGameController()) {
                if (gc->ChatWidget) {
                    gc->ChatWidget->Print(msg, col);
                }
            }
        }

        // Etat local des recrues (GUIDs)
        std::vector<uint64_t> g_party;

        // Heuristique très simple : cible “proche” du joueur
        cube::Creature* FindRecruitCandidate() {
            auto* gc = cube::GetGameController();
            if (!gc || !gc->local_player) return nullptr;

            // Position joueur (X/Y/Z en MAJUSCULES dans ce CWSDK)
            const auto& ppos = gc->local_player->entity_data.position;

            std::map<uint64_t, cube::Creature*> snap;
            SnapshotCreatures(gc->world, snap);

            cube::Creature* best = nullptr;
            int64_t bestDist2 = (int64_t)1e18;

            for (auto& [id, c] : snap) {
                if (!c || c == gc->local_player) continue;

                // Ignore déjà recrutés (par owner) et hostiles
                if (GetParentOwner(c) == gc->local_player->guid) continue;
                if (GetHostility(c) & 1u /*hostile bit*/) continue;

                const auto& epos = c->entity_data.position;
                const int64_t dx = epos.X - ppos.X;
                const int64_t dy = epos.Y - ppos.Y;
                const int64_t dz = epos.Z - ppos.Z;
                const int64_t d2 = dx * dx + dy * dy + dz * dz;

                if (d2 < bestDist2) {
                    bestDist2 = d2;
                    best = c;
                }
            }
            return best;
        }

    } // namespace anonyme

    // ================= API exposée à dllmain =================

    void Init() {
        Chat(L"[RecruitMod] Init()", Color::Green());
    }

    void Tick() {
        // battement de cœur discret (utile pour confirmer la présence du mod)
        static int counter = 0;
        if ((++counter % 600) == 0) {
            Chat(L"[RecruitMod] heartbeat", Color::Blue());
        }

        auto* gc = cube::GetGameController();
        if (!gc) return;

        // F6 recrute
        if (GetAsyncKeyState(VK_F6) & 1) {
            Chat(L"[RecruitMod] Hotkey F6", Color::White());
            CmdRecruit();
        }
        // F7 dismiss
        if (GetAsyncKeyState(VK_F7) & 1) {
            Chat(L"[RecruitMod] Hotkey F7", Color::White());
            CmdDismiss();
        }
        // F8 party
        if (GetAsyncKeyState(VK_F8) & 1) {
            Chat(L"[RecruitMod] Hotkey F8", Color::White());
            CmdParty();
        }
    }

    void CmdRecruit() {
        auto* gc = cube::GetGameController();
        if (!gc || !gc->local_player) {
            Chat(L"[RecruitMod] pas de GameController / joueur", Color::Red());
            return;
        }

        cube::Creature* cand = FindRecruitCandidate();
        if (!cand) {
            Chat(L"[RecruitMod] aucun candidat valide à proximité", Color::White());
            return;
        }

        // Marque le propriétaire (si le champ existe dans ce layout)
        if constexpr (decltype(test_parent_owner<cube::Creature>(0))::value) {
            cand->parent_owner = gc->local_player->guid;
        }
        else if constexpr (decltype(test_entity_data_parent_owner<cube::Creature>(0))::value) {
            cand->entity_data.parent_owner = gc->local_player->guid;
        }

        g_party.push_back(cand->guid);
        Chat(L"[RecruitMod] Recruté ✔", Color::Green());
    }

    void CmdDismiss() {
        auto* gc = cube::GetGameController();
        if (!gc) return;

        if (g_party.empty()) {
            Chat(L"[RecruitMod] Aucun compagnon à renvoyer.", Color::White());
            return;
        }

        std::map<uint64_t, cube::Creature*> snap;
        SnapshotCreatures(gc->world, snap);

        for (auto id : g_party) {
            if (auto it = snap.find(id); it != snap.end() && it->second) {
                auto* c = it->second;
                // enlève l’owner si possible
                if constexpr (decltype(test_parent_owner<cube::Creature>(0))::value) {
                    c->parent_owner = -1;
                }
                else if constexpr (decltype(test_entity_data_parent_owner<cube::Creature>(0))::value) {
                    c->entity_data.parent_owner = -1;
                }
            }
        }
        g_party.clear();
        Chat(L"[RecruitMod] Compagnons renvoyés.", Color::Red());
    }

    void CmdParty() {
        if (g_party.empty()) {
            Chat(L"[RecruitMod] Party vide.", Color::White());
            return;
        }

        // Affiche simplement le nombre (safe tant qu’on n’a pas de noms)
        wchar_t buf[128];
        _snwprintf_s(buf, _TRUNCATE, L"[RecruitMod] Party: %zu compagnon(s).", g_party.size());
        Chat(buf, Color::Blue());
    }

} // namespace RecruitMod
