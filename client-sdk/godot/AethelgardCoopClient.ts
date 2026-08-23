/*
 * AETHELGRAD Godot TypeScript networking wrapper.
 *
 * Compatible with Godot TypeScript runtimes that expose the standard fetch API.
 * Keep access and refresh tokens in platform-secure storage in production.
 */

export type AuthBundle = {
  accessToken: string;
  refreshToken: string;
  tokenType: "Bearer";
  accountId: string;
  accountType?: "guest" | "google";
  expiresAt: string;
  refreshExpiresAt: string;
};

export type Participant = {
  accountId: string;
  playerX: number;
  playerY: number;
  atTower: boolean;
  towerRevision: number;
};

export type RoomSnapshot = {
  code: string;
  region: string;
  maxPlayers: number;
  worldTime: number;
  towerRevision: number;
  bossHealth: number;
  combatRevision: number;
  participants: Participant[];
};

export type RoomResponse = {
  room: RoomSnapshot;
  participants: Participant[];
};

export type CombatResult = {
  accepted: boolean;
  action: "attack" | "heavy_attack";
  targetId: string;
  damage: number;
  bossHealth: number;
  combatRevision: number;
};

export type InventoryState = {
  wood: number;
  fiber: number;
  stone: number;
  emberKit: boolean;
};

export type InventoryResult = {
  accepted: boolean;
  operation: "gather" | "craft";
  inventory: InventoryState;
  inventoryRevision: number;
};

export type HeartbeatInput = {
  playerX: number;
  playerY: number;
  atTower: boolean;
  towerRevision: number;
};

export type ClientOptions = {
  baseUrl: string;
  accessToken?: string;
  refreshToken?: string;
  fetchImpl?: typeof fetch;
  onAuthBundle?: (bundle: AuthBundle) => void;
  onRoomSnapshot?: (response: RoomResponse) => void;
};

export class AethelgardApiError extends Error {
  readonly status: number;
  readonly code: string;

  constructor(status: number, code: string) {
    super(`AETHELGRAD API ${status}: ${code}`);
    this.name = "AethelgardApiError";
    this.status = status;
    this.code = code;
  }
}

export class AethelgardCoopClient {
  private readonly baseUrl: string;
  private readonly fetchImpl: typeof fetch;
  private accessToken: string | null;
  private refreshToken: string | null;
  private requestCounter = 0;
  private readonly onAuthBundle?: (bundle: AuthBundle) => void;
  private readonly onRoomSnapshot?: (response: RoomResponse) => void;

  constructor(options: ClientOptions) {
    this.baseUrl = options.baseUrl.replace(/\/$/, "");
    this.fetchImpl = options.fetchImpl ?? fetch;
    this.accessToken = options.accessToken ?? null;
    this.refreshToken = options.refreshToken ?? null;
    this.onAuthBundle = options.onAuthBundle;
    this.onRoomSnapshot = options.onRoomSnapshot;
  }

  setTokens(bundle: Pick<AuthBundle, "accessToken" | "refreshToken">): void {
    this.accessToken = bundle.accessToken;
    this.refreshToken = bundle.refreshToken;
  }

  clearTokens(): void {
    this.accessToken = null;
    this.refreshToken = null;
  }

  async exchangeGoogleIdToken(idToken: string): Promise<AuthBundle> {
    const bundle = await this.request<AuthBundle>("POST", "/v1/auth/google-id-token/exchange", { idToken }, false);
    this.setTokens(bundle);
    this.onAuthBundle?.(bundle);
    return bundle;
  }

  async authenticateGuest(guestKey: string): Promise<AuthBundle> {
    if (!/^[A-Za-z0-9_-]{32,128}$/.test(guestKey)) throw new Error("guestKey must be a base64url secret with 32-128 characters");
    const bundle = await this.request<AuthBundle>("POST", "/v1/auth/guest", { guestKey }, false);
    this.setTokens(bundle);
    this.onAuthBundle?.(bundle);
    return bundle;
  }

  async refresh(): Promise<AuthBundle> {
    if (!this.refreshToken) throw new AethelgardApiError(401, "missing_refresh_token");
    const bundle = await this.request<AuthBundle>("POST", "/v1/auth/refresh", { refreshToken: this.refreshToken }, false);
    this.setTokens(bundle);
    this.onAuthBundle?.(bundle);
    return bundle;
  }

  async createRoom(region = "asia"): Promise<RoomResponse> {
    return this.roomRequest("POST", "/v1/coop/rooms", { region });
  }

  async joinRoom(roomCode: string): Promise<RoomResponse> {
    return this.roomRequest("POST", `/v1/coop/rooms/${this.normalizeRoomCode(roomCode)}/join`, {});
  }

  async getRoom(roomCode: string): Promise<RoomResponse> {
    return this.roomRequest("GET", `/v1/coop/rooms/${this.normalizeRoomCode(roomCode)}`);
  }

  async heartbeat(roomCode: string, input: HeartbeatInput): Promise<RoomResponse> {
    const payload: HeartbeatInput = {
      playerX: this.clamp(input.playerX, -0.9, 0.9),
      playerY: this.clamp(input.playerY, -0.5, 0.52),
      atTower: Boolean(input.atTower),
      towerRevision: Math.max(0, Math.floor(input.towerRevision)),
    };
    return this.roomRequest("POST", `/v1/coop/rooms/${this.normalizeRoomCode(roomCode)}/heartbeat`, payload);
  }

  async leaveRoom(roomCode: string): Promise<void> {
    await this.request<void>("DELETE", `/v1/coop/rooms/${this.normalizeRoomCode(roomCode)}/leave`);
  }

  async combat(roomCode: string, action: "attack" | "heavy_attack", targetId = "forest_warden", requestId = this.nextRequestId("combat")): Promise<CombatResult> {
    if (!/^[A-Za-z0-9_-]{8,80}$/.test(requestId)) throw new Error("requestId must match [A-Za-z0-9_-]{8,80}");
    return this.request<CombatResult>("POST", `/v1/coop/rooms/${this.normalizeRoomCode(roomCode)}/combat`, { requestId, action, targetId });
  }

  async gather(roomCode: string, resourceId: "forest_cache" | "root_cache" | "warden_stone", requestId = this.nextRequestId("gather")): Promise<InventoryResult> {
    return this.inventory(roomCode, { requestId, operation: "gather", resourceId });
  }

  async craft(roomCode: string, requestId = this.nextRequestId("craft")): Promise<InventoryResult> {
    return this.inventory(roomCode, { requestId, operation: "craft" });
  }

  private async inventory(roomCode: string, body: Record<string, string>): Promise<InventoryResult> {
    if (!/^[A-Za-z0-9_-]{8,80}$/.test(body.requestId)) throw new Error("requestId must match [A-Za-z0-9_-]{8,80}");
    return this.request<InventoryResult>("POST", `/v1/coop/rooms/${this.normalizeRoomCode(roomCode)}/inventory`, body);
  }

  private async roomRequest(method: string, path: string, body?: unknown): Promise<RoomResponse> {
    const response = await this.request<RoomResponse>(method, path, body);
    this.onRoomSnapshot?.(response);
    return response;
  }

  private async request<T>(method: string, path: string, body?: unknown, allowRefresh = true): Promise<T> {
    const headers: Record<string, string> = { Accept: "application/json" };
    if (body !== undefined) headers["Content-Type"] = "application/json";
    if (this.accessToken) headers.Authorization = `Bearer ${this.accessToken}`;

    const response = await this.fetchImpl(`${this.baseUrl}${path}`, {
      method,
      headers,
      body: body === undefined ? undefined : JSON.stringify(body),
    });

    if (response.ok) {
      if (response.status === 204) return undefined as T;
      return (await response.json()) as T;
    }

    const payload = await response.json().catch(() => ({ error: "http_error" }));
    const error = new AethelgardApiError(response.status, String(payload.error ?? "http_error"));
    if (allowRefresh && response.status === 401 && this.refreshToken && path !== "/v1/auth/refresh") {
      await this.refresh();
      return this.request<T>(method, path, body, false);
    }
    throw error;
  }

  private normalizeRoomCode(roomCode: string): string {
    const normalized = roomCode.trim().toUpperCase();
    if (!/^[A-Z0-9]{6}$/.test(normalized)) throw new Error("roomCode must be six uppercase letters or digits");
    return normalized;
  }

  private nextRequestId(prefix: string): string {
    this.requestCounter += 1;
    return `${prefix}-${Date.now()}-${this.requestCounter}`;
  }

  private clamp(value: number, min: number, max: number): number {
    return Math.min(max, Math.max(min, Number.isFinite(value) ? value : min));
  }
}
