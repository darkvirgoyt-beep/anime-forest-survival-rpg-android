using System;
using System.Collections;
using System.Threading.Tasks;
using UnityEngine;
using UnityEngine.Networking;

namespace Aethelgrad.Net
{
    [Serializable]
    public sealed class AuthBundle
    {
        public string accessToken;
        public string refreshToken;
        public string tokenType;
        public string accountId;
        public string accountType;
        public string expiresAt;
        public string refreshExpiresAt;
    }

    [Serializable]
    public sealed class Participant
    {
        public string accountId;
        public float playerX;
        public float playerY;
        public bool atTower;
        public int towerRevision;
    }

    [Serializable]
    public sealed class RoomSnapshot
    {
        public string code;
        public string region;
        public int maxPlayers;
        public float worldTime;
        public int towerRevision;
        public int bossHealth;
        public int combatRevision;
    }

    [Serializable]
    public sealed class RoomResponse
    {
        public RoomSnapshot room;
        public Participant[] participants;
    }

    [Serializable]
    public sealed class CombatResult
    {
        public bool accepted;
        public string action;
        public string targetId;
        public int damage;
        public int bossHealth;
        public int combatRevision;
    }

    [Serializable]
    public sealed class InventoryState
    {
        public int wood;
        public int fiber;
        public int stone;
        public bool emberKit;
    }

    [Serializable]
    public sealed class InventoryResult
    {
        public bool accepted;
        public string operation;
        public InventoryState inventory;
        public int inventoryRevision;
    }

    [Serializable]
    private sealed class ErrorResponse
    {
        public string error;
    }

    [Serializable]
    private sealed class GoogleIdTokenRequest
    {
        public string idToken;
        public GoogleIdTokenRequest(string value) { idToken = value; }
    }

    [Serializable]
    private sealed class GuestKeyRequest
    {
        public string guestKey;
        public GuestKeyRequest(string value) { guestKey = value; }
    }

    [Serializable]
    private sealed class RefreshRequest
    {
        public string refreshToken;
        public RefreshRequest(string value) { refreshToken = value; }
    }

    [Serializable]
    private sealed class RegionRequest
    {
        public string region;
        public RegionRequest(string value) { region = value; }
    }

    [Serializable]
    private sealed class HeartbeatRequest
    {
        public float playerX;
        public float playerY;
        public bool atTower;
        public int towerRevision;
    }

    [Serializable]
    private sealed class CombatRequest
    {
        public string requestId;
        public string action;
        public string targetId;
    }

    [Serializable]
    private sealed class InventoryRequest
    {
        public string requestId;
        public string operation;
        public string resourceId;
    }

    public sealed class AethelgardApiException : Exception
    {
        public long StatusCode { get; private set; }
        public string ErrorCode { get; private set; }

        public AethelgardApiException(long statusCode, string errorCode)
            : base($"AETHELGRAD API {statusCode}: {errorCode}")
        {
            StatusCode = statusCode;
            ErrorCode = errorCode;
        }
    }

    public sealed class AethelgardCoopClient
    {
        private readonly string baseUrl;
        private string accessToken;
        private string refreshToken;
        private int requestCounter;

        public event Action<AuthBundle> AuthBundleReceived;
        public event Action<RoomResponse> RoomSnapshotReceived;

        public AethelgardCoopClient(string serviceBaseUrl, string existingAccessToken = null, string existingRefreshToken = null)
        {
            baseUrl = serviceBaseUrl.TrimEnd('/');
            accessToken = existingAccessToken;
            refreshToken = existingRefreshToken;
        }

        public void SetTokens(AuthBundle bundle)
        {
            accessToken = bundle.accessToken;
            refreshToken = bundle.refreshToken;
        }

        public void ClearTokens()
        {
            accessToken = null;
            refreshToken = null;
        }

        public async Task<AuthBundle> ExchangeGoogleIdTokenAsync(string idToken)
        {
            AuthBundle bundle = await SendJsonAsync<AuthBundle>("POST", "/v1/auth/google-id-token/exchange", new GoogleIdTokenRequest(idToken), false);
            SetTokens(bundle);
            AuthBundleReceived?.Invoke(bundle);
            return bundle;
        }

        public async Task<AuthBundle> AuthenticateGuestAsync(string guestKey)
        {
            if (string.IsNullOrEmpty(guestKey) || !System.Text.RegularExpressions.Regex.IsMatch(guestKey, "^[A-Za-z0-9_-]{32,128}$"))
                throw new ArgumentException("guestKey must be a base64url secret with 32-128 characters");
            AuthBundle bundle = await SendJsonAsync<AuthBundle>("POST", "/v1/auth/guest", new GuestKeyRequest(guestKey), false);
            SetTokens(bundle);
            AuthBundleReceived?.Invoke(bundle);
            return bundle;
        }

        public async Task<AuthBundle> RefreshAsync()
        {
            if (string.IsNullOrEmpty(refreshToken)) throw new AethelgardApiException(401, "missing_refresh_token");
            AuthBundle bundle = await SendJsonAsync<AuthBundle>("POST", "/v1/auth/refresh", new RefreshRequest(refreshToken), false);
            SetTokens(bundle);
            AuthBundleReceived?.Invoke(bundle);
            return bundle;
        }

        public Task<RoomResponse> CreateRoomAsync(string region = "asia")
        {
            return RoomRequestAsync("POST", "/v1/coop/rooms", new RegionRequest(region));
        }

        public Task<RoomResponse> JoinRoomAsync(string roomCode)
        {
            return RoomRequestAsync("POST", "/v1/coop/rooms/" + NormalizeRoomCode(roomCode) + "/join", new object());
        }

        public Task<RoomResponse> GetRoomAsync(string roomCode)
        {
            return RoomRequestAsync("GET", "/v1/coop/rooms/" + NormalizeRoomCode(roomCode), null);
        }

        public Task<RoomResponse> HeartbeatAsync(string roomCode, float playerX, float playerY, bool atTower, int towerRevision)
        {
            HeartbeatRequest payload = new HeartbeatRequest
            {
                playerX = Mathf.Clamp(playerX, -0.90f, 0.90f),
                playerY = Mathf.Clamp(playerY, -0.50f, 0.52f),
                atTower = atTower,
                towerRevision = Mathf.Max(0, towerRevision)
            };
            return RoomRequestAsync("POST", "/v1/coop/rooms/" + NormalizeRoomCode(roomCode) + "/heartbeat", payload);
        }

        public async Task LeaveRoomAsync(string roomCode)
        {
            await SendJsonAsync<object>("DELETE", "/v1/coop/rooms/" + NormalizeRoomCode(roomCode) + "/leave", null);
        }

        public Task<CombatResult> CombatAsync(string roomCode, string action, string targetId = "forest_warden", string requestId = null)
        {
            if (action != "attack" && action != "heavy_attack") throw new ArgumentException("action must be attack or heavy_attack");
            CombatRequest payload = new CombatRequest
            {
                requestId = requestId ?? NextRequestId("combat"),
                action = action,
                targetId = targetId
            };
            return SendJsonAsync<CombatResult>("POST", "/v1/coop/rooms/" + NormalizeRoomCode(roomCode) + "/combat", payload);
        }

        public Task<InventoryResult> GatherAsync(string roomCode, string resourceId, string requestId = null)
        {
            if (resourceId != "forest_cache" && resourceId != "root_cache" && resourceId != "warden_stone") throw new ArgumentException("Unknown resourceId");
            InventoryRequest payload = new InventoryRequest
            {
                requestId = requestId ?? NextRequestId("gather"),
                operation = "gather",
                resourceId = resourceId
            };
            return SendJsonAsync<InventoryResult>("POST", "/v1/coop/rooms/" + NormalizeRoomCode(roomCode) + "/inventory", payload);
        }

        public Task<InventoryResult> CraftAsync(string roomCode, string requestId = null)
        {
            InventoryRequest payload = new InventoryRequest
            {
                requestId = requestId ?? NextRequestId("craft"),
                operation = "craft",
                resourceId = null
            };
            return SendJsonAsync<InventoryResult>("POST", "/v1/coop/rooms/" + NormalizeRoomCode(roomCode) + "/inventory", payload);
        }

        private async Task<RoomResponse> RoomRequestAsync(string method, string path, object body)
        {
            RoomResponse response = await SendJsonAsync<RoomResponse>(method, path, body);
            RoomSnapshotReceived?.Invoke(response);
            return response;
        }

        private async Task<T> SendJsonAsync<T>(string method, string path, object body, bool allowRefresh = true)
        {
            string payload = body == null ? null : JsonUtility.ToJson(body);
            using (UnityWebRequest request = new UnityWebRequest(baseUrl + path, method))
            {
                if (payload != null)
                {
                    byte[] bytes = System.Text.Encoding.UTF8.GetBytes(payload);
                    request.uploadHandler = new UploadHandlerRaw(bytes);
                    request.SetRequestHeader("Content-Type", "application/json");
                }
                request.downloadHandler = new DownloadHandlerBuffer();
                request.SetRequestHeader("Accept", "application/json");
                if (!string.IsNullOrEmpty(accessToken)) request.SetRequestHeader("Authorization", "Bearer " + accessToken);

                UnityWebRequestAsyncOperation operation = request.SendWebRequest();
                while (!operation.isDone) await Task.Yield();

                if (request.result == UnityWebRequest.Result.Success && request.responseCode >= 200 && request.responseCode < 300)
                {
                    if (request.responseCode == 204 || typeof(T) == typeof(object)) return default(T);
                    return JsonUtility.FromJson<T>(request.downloadHandler.text);
                }

                ErrorResponse error = null;
                try { error = JsonUtility.FromJson<ErrorResponse>(request.downloadHandler.text); } catch { }
                string errorCode = string.IsNullOrEmpty(error?.error) ? "http_error" : error.error;
                if (allowRefresh && request.responseCode == 401 && !string.IsNullOrEmpty(refreshToken) && path != "/v1/auth/refresh")
                {
                    await RefreshAsync();
                    return await SendJsonAsync<T>(method, path, body, false);
                }
                throw new AethelgardApiException(request.responseCode, errorCode);
            }
        }

        private string NormalizeRoomCode(string roomCode)
        {
            string normalized = roomCode.Trim().ToUpperInvariant();
            if (normalized.Length != 6) throw new ArgumentException("roomCode must contain six characters");
            for (int i = 0; i < normalized.Length; i++)
            {
                char c = normalized[i];
                if (!(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9')) throw new ArgumentException("roomCode must be alphanumeric");
            }
            return normalized;
        }

        private string NextRequestId(string prefix)
        {
            requestCounter++;
            return $"{prefix}-{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}-{requestCounter}";
        }
    }
}
